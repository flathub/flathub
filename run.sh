#!/bin/sh

# Check if the script was called with any arguments
if [ "$#" -eq 0 ]; then
    echo -e "\033[44;1;37mWelcome to Corese-Command!\033[0m"
    echo -e "\n\033[1mQuick Setup Guide:\033[0m\n"

    echo -e "\033[1;34mStep 1:\033[0m Open a terminal window.\n"
    
    echo -e "\033[1;34mStep 2:\033[0m Add the alias to your shell's configuration file for permanent use:"
    
    # Bash instruction
    echo -e "  \033[1;36m- For Bash:\033[0m"
    echo -e "  \033[1;33mecho \"alias corese-command='flatpak run fr.inria.corese.CoreseCommand'\" >> ~/.bashrc\033[0m"
    
    # Zsh instruction
    echo -e "  \033[1;36m- For Zsh:\033[0m"
    echo -e "  \033[1;33mecho \"alias corese-command='flatpak run fr.inria.corese.CoreseCommand'\" >> ~/.zshrc\033[0m"

    # Fish instruction
    echo -e "  \033[1;36m- For Fish:\033[0m"
    echo -e "  \033[1;33malias corese-command='flatpak run fr.inria.corese.CoreseCommand'; funcsave corese-command\033[0m\n"

    echo -e "\033[1;34mStep 3:\033[0m To use Corese-Command, simply type 'corese-command' followed by any options. For example:"
    echo -e "\033[1;33mcorese-command -h\033[0m\n"


    echo -e "Press any key to close this terminal window."
  
    # Wait for user input
    read -n 1 -s
else
    exec /app/jre/bin/java -jar /app/bin/corese-command.jar "$@"
fi
