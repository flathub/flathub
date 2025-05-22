
import subprocess
import os
import mimetypes
import logging

logger = logging.getLogger(__name__)


def get_installed_packages():
    try:
        # Exécutez la commande "pip freeze" via subprocess
        result = subprocess.run(
            ['pip3', 'freeze'], capture_output=True, text=True, check=True)
        # Récupérez et retournez la sortie de la commande
        return result.stdout
    except subprocess.CalledProcessError as e:
        logger.error(f"Error: {e}")
        return None

# Appel de la fonction pour obtenir la liste des paquets installés
# installed_packages = get_installed_packages()
# print(installed_packages)


def afficher_contenu_repertoire(chemin):
    # Vérifier si le chemin spécifié est un répertoire
    if os.path.isdir(chemin):
        # Liste tous les éléments dans le répertoire
        contenu = os.listdir(chemin)

        # Afficher chaque élément
        for element in contenu:
            print(element)
    else:
        logger.error("The specified path is not a directory.")

def find_key(target_value: str, dictionary):
    '''find the key associated with the value in the dictionary'''
    found_key = None
    for key, value in dictionary.items():
        if value == target_value:
            found_key = key
            break
    return found_key


def open_file_manager(path: str) -> None:
    """
    Opens the file manager to the specified directory path.

    This function detects the operating system and uses the appropriate command
    to open the file manager. For Windows, it uses the Explorer. For Linux, it
    uses the D-Bus interface to open the file manager.

    Parameters:
        path (str): The directory path to open in the file manager.

    Raises:
        OSError: If the operating system is unsupported.

    Examples:
        >>> open_file_manager("/path/to/directory")
    """

    if os.name == 'nt':  # For Windows
        import subprocess

        subprocess.run(['explorer', path])

    elif os.name == 'posix':  # For Linux
        from pydbus import SessionBus

        bus = SessionBus()
        file_manager = bus.get("org.freedesktop.FileManager1")
        file_manager.ShowFolders([f"file://{path}"], "")
    else:
        raise OSError("Unsupported operating system")


def get_mime_type(self, file_path):
    """
    Determine the MIME type associated with a given file name or path.

    Parameters:
    - file_path (str): The file extension, such as 'txt', 'jpg', or 'pdf'.

    Returns:
    - str: The MIME type of the specified file extension. If the extension
        is not recognized, returns 'unknown'.
    """

    mime_type, _ = mimetypes.guess_type(file_path)
    return mime_type if mime_type else "unknown"
