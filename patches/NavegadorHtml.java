// Patch description:
//
// This fixes URLs not opening with the default web browser.
// Previously, we would work around this problem by using the GNOME runtime and
// adding a few extra permissions to the manifest.
// However, this proved to be an insuficient solution in some OSes, because they'd
// also require gvfs to be installed on the host for this to work.
//
// This class should override the one in: lib-modulos/irpf_gui-basica.jar (serpro/ppgd/irpf/gui)

package serpro.ppgd.irpf.gui;

import java.io.IOException;
import java.net.URI;

public class NavegadorHtml {
    public static void executarNavegadorComMsgErro(URI uri) {
        try {
            NavegadorHtml.executarNavegador(uri);
        }
        catch (IOException e) {
            System.out.println("ERROR: Failed to open URL with xdg-open: " + uri.toString());
        }
    }

    public static void executarNavegador(String uri) throws IOException {
        NavegadorHtml.executarNavegador(URI.create(uri));
    }

    public static void executarNavegadorComMsgErro(String uri) {
        NavegadorHtml.executarNavegadorComMsgErro(URI.create(uri));
    }

    public static void executarNavegador(URI uri) throws IOException {
        System.out.println("INFO: Opening URL with xdg-open: " + uri.toString());
        try {
            Process p = Runtime.getRuntime().exec(new String[] { "xdg-open", uri.toString() });
            p.waitFor();
            p.destroy();
        } catch (InterruptedException e) { }
    }
}
