#!/usr/bin/bash

#####
:   '
# Overview :
# ---------
# @overview The module (progress-bar-cut.sh) is a module that is composed a certain number 
# of methods allowing to display the status of transfer of a file or directory.
#
# Author :
# -------
# @author Baldé Amadou (baldeuniversel@protonmail.com)
#
# Version :
# --------
# @version 3.1.2 (15 February 2024)
    '
#####


set -uo pipefail


# Declaration variables
a_source_data=$1
a_destination_dir_target_embed=$2

a_length_ongoing_data=0
a_size_source=0
a_percent_stat_bar=0
a_new_percent=0
a_old_percent=0

a_getThisPid="$$"

a_flagSIGTERM="FALSE"
a_terminate_process="TRUE"

declare -a a_character_bar_front_list=("▊" "▉" "█")
a_character_bar_back="-"
a_filePidCommandMv="$HOME/.local/share/am-okay/classic/classic-pid-mv"

tmp_sizeOfSourceData="/tmp/.$USER/am-okay/progress/$a_getThisPid/size-source-data"
tmp_sizeOfOngoingData="/tmp/.$USER/am-okay/progress/$a_getThisPid/size-ongoing-data"
tmp_flagSourceComputed="/tmp/.$USER/am-okay/progress/$a_getThisPid/flag-source-computed"
tmp_flagNextOngoingComputed="/tmp/.$USER/am-okay/progress/$a_getThisPid/flag-next-ongoing-computed"

# Create a tmp directory for this process (Goal -> running processes in parallel)
if [[ -e "/tmp/.$USER/am-okay/progress/$a_getThisPid" ]]
then
    #
    rm -fr "/tmp/.$USER/am-okay/progress/$a_getThisPid" 2> /dev/null

    #
    mkdir -p "/tmp/.$USER/am-okay/progress/$a_getThisPid" 2> /dev/null
else
    #
    mkdir -p "/tmp/.$USER/am-okay/progress/$a_getThisPid" 2> /dev/null
fi


#
a_getThePidCommandMv=` cat $a_filePidCommandMv 2> /dev/null | tr -d "[[:space:]]" `





:   '
@constructor
# 
# Author :
# -------
# @author Baldé Amadou (baldeuniversel@protonmail.com)
    '
function __init__ 
{
    #
    echo -ne ""
}





:   '
# @destructor
# 
# Author :
# -------
# @author Baldé Amadou (baldeuniversel@protonmail.com)
    '
function __del__ 
{
    rm -rf "/tmp/.$USER/am-okay/progress/$a_getThisPid" 2> /dev/null 
    
    #
    rm -rf "$a_filePidCommandMv" 2> /dev/null 

    echo -e "\n"
}





:   '
# @method
# 
# Parameter :
# ----------
# :param <$1> type(str) // The file or directory whom the size has to be calculated
# 
# Return : 
# -------
# :return The size of the file or directory
# 
# Author :
# -------
# @author Baldé Amadou (baldeuniversel@protonmail.com)
    '
function get_size 
{
    # Declaration local variables
    local v_total_size=0
    local target_elem="$1"

    # Get the size of target dir/file
    if [[ -e "$target_elem" ]]
    then
        v_total_size=`du -sb "$target_elem" | tr -s "[[:space:]]" ":" | cut -d ":" -f1`
    fi

    # print the total size of the directory
    printf "%d" $v_total_size
}





:   '
/**
* @overview The function `isFileDirSameDisk` allows to know if two files/dirs or dirs/files
* belong to the same disk 
*
* @param {string} $1 // The source dir/file
* @param {string} $2 // The target directory
*
* @return {string} // A `true` string value will be returned if the two inputs belong
*                     to the same disk , otherwise `false` value will be returned 
*/
'
function isFileDirSameDisk
{
    # Declaration local variables
    local getSource="$1"
    local getDest=` echo "$2" | awk '{ split($0, arr, "/"); for (i = 1; i < length(arr); i++) \
        { if (i == 0) { printf("/%s", arr[i]) } else { printf("%s/", arr[i]) } } }' `



    # Check to see if the files or dirs belong to the same disk 
    if [[  ` stat -c "%d" "$getSource" 2> /dev/null ` -eq  ` stat -c "%d" "$getDest" 2> /dev/null ` ]]
    then
        echo "true"
    else
        echo "false"
    fi
}







:   '
/**
* @overview The function `setFlagSIGINT` allows to change the value of the variable `a_flagSIGTERM` , 
* in this sense with the `trap` command there will be a control at the level of the progress bar 
*/
    '
function setFlagSIGINT
{
    # Change the value of the variable `a_flagSIGTERM` to `TRUE`
    a_flagSIGTERM="TRUE"
   

    #
    kill -9 $a_getThePidCommandMv &> /dev/null

    #
    rm -rf "$a_filePidCommandMv" &> /dev/null
    
    #
    rm -rf "/tmp/.$USER/am-okay/progress/$a_getThisPid" 2> /dev/null

    #
    exit 1
}





:   '
/**
* @overview The function `byteTransformer` allows to transform bytes in kbytes 
* or in Mbytes ... (according the size of the input)
* 
* @param {int} $1 // The size of the data 
* 
* @return {string} // A string containing a new(or not) size and a 
* new(nor not) unit will be returned
*/
    '
function byteTransformer
{
    # Declaration variable
    local getTheSize="$1"
    local theNewSize=0
    local integerPart=0
    local fractionalPart=0
    local quotient=0


    #
    if [[ $getTheSize -lt 100 ]]
    then   
        #
        echo -e "$getTheSize:B"
    #
    elif [[ $getTheSize -ge 100 ]] && [[ $getTheSize -lt 100000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        # Treatment on integer part 
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi

        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi

        #
        theNewSize="${integerPart}.${fractionalPart}"
        
        #
        echo -e "$theNewSize:KB"
    #
    elif [[ $getTheSize -ge 100000 ]] && [[ $getTheSize -lt 100000000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        #
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi

    
        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi


        #
        theNewSize="${integerPart}.${fractionalPart}"

        #
        echo -e "$theNewSize:MB"
    #
    elif [[ $getTheSize -ge 100000000 ]] && [[ $getTheSize -lt 100000000000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000000000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        #
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi

        
        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi


        #
        theNewSize="${integerPart}.${fractionalPart}"

        #
        echo "$theNewSize:GB"

    elif [[ $getTheSize -ge 100000000000 ]] && [[ $getTheSize -lt 100000000000000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000000000000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        #
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi

        
        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi


        #
        theNewSize="${integerPart}.${fractionalPart}"

        #
        echo "$theNewSize:TB"
    #
    elif [[ $getTheSize -ge 100000000000000 ]] && [[ $getTheSize -lt 100000000000000000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000000000000000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        #
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi

        
        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi


        #
        theNewSize="${integerPart}.${fractionalPart}"

        #
        echo "$theNewSize:PB"
    #
    elif [[ $getTheSize -ge 100000000000000000 ]] && [[ $getTheSize -lt 100000000000000000000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000000000000000000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        #
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi

        
        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi


        #
        theNewSize="${integerPart}.${fractionalPart}"

        #
        echo "$theNewSize:EB"
    #
    elif [[ $getTheSize -ge 100000000000000000000 ]]
    then
        #
        quotient=` echo "$getTheSize/1000000000000000000000" | bc -l `

        #
        integerPart=` echo "$quotient" | cut -d "." -f 1 `
        fractionalPart=` echo "$quotient" | cut -d "." -f 2 | awk '{ print substr($0, 1, 2) }' `

        #
        if [[ ! ( -n $integerPart ) ]]
        then 
            #
            integerPart=0
        fi


        # Treatment on the fractional part
        if [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -eq 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="0?"
        #
        elif [[ ` echo "${fractionalPart:0:1}" 2> /dev/null ` -gt 0 ]] && [[ ` echo "${fractionalPart:1:1}" 2> /dev/null ` -eq 0 ]]
        then 
            #
            fractionalPart="${fractionalPart:0:1}?"
        fi


        theNewSize="${integerPart}.${fractionalPart}"

        #
        echo "$theNewSize:ZB"
    fi
}





:   '
# @method
# 
# Overview :
# ---------
# @overview Display the progress bar by using in part the methods of the module <<progress-bar-copy.sh>>
# 
# Author :
# -------
# @author Baldé Amadou (baldeuniversel@protonmail.com)
    '
function display_progress_bar
{
    # Declaration variables
    local decrement_bar_back=0
    local counter=0
    
    local getSizeOfSourceData=0
    local getSizeOfOngoingData=0
    local counterSourceComputed=0
    local percent_stat_ongoing=0

    local getSizeLinkedUnitSrc=0
    local getSizeLinkedUnitOngoing=0
    local getUnitSrc=0
    local getUnitOngoing=0
    local getSizeAndUnitTmp=""

    local getPidCommandMv=` cat "$a_filePidCommandMv" 2> /dev/null `

    local source_dir_file=` echo "$a_source_data" | awk -F '/' '{ print $NF }' `
    local destination_dir_file="$a_destination_dir_target_embed"


    # Call the constructor <<__init__>>
    __init__



    # Action to get the two last directories if there is more than three directories
    for (( i=0; i < ${#destination_dir_file}; i++ ))
    do
        if [[ ${destination_dir_file:i:1} == "/" ]]
        then
            counter=$(( counter + 1 ))
        fi

        #
        if [[ $counter -gt 2 ]]
        then

            destination_dir_file=` echo $destination_dir_file \
                | awk -F'/' '{ split($0, arr, "/"); print arr[length(arr)-1] "/" arr[length(arr)] }' `

            destination_dir_file=` echo "...$destination_dir_file" `

            break # Stop loop
        fi
    done   


    #
    echo -e "\e[1;036m Cut ~\e[0m $source_dir_file -> $destination_dir_file"

    # Print backward char in white color
    echo -en "\033[37m|"
    #
    for counter in {1..20}
    do
        echo -en "$a_character_bar_back"
    done

    # Reset the white color
    echo -en "| \e[1;036m$percent_stat_ongoing%\e[0m \033[0m"


    # To allow the calculation (the size) of the data sent (in parallel)
    echo "true" > "$tmp_flagNextOngoingComputed"

    # Get the size of the source data -> call the function <<get_size>>
    ( 
        getSizeOfSourceData=$(get_size $a_source_data) 

        echo "$getSizeOfSourceData" > "$tmp_sizeOfSourceData"

        echo "true" > "$tmp_flagSourceComputed"
    ) &


 
    #
    while [[ $a_terminate_process == "TRUE" ]]
    do  
        #
        if [[ -e "$tmp_sizeOfSourceData" ]] && [[ ` cat "$tmp_flagSourceComputed" 2> /dev/null | grep -w -- "true" ` ]] \
            && [[ $counterSourceComputed -eq 0 ]]
        then
            #
            a_size_source=` cat "$tmp_sizeOfSourceData" 2> /dev/null | tr -d "[[:space:]]" `

            # Call the function
            getSizeAndUnitTmp=` byteTransformer $a_size_source `

            #
            getSizeLinkedUnitSrc=` echo "$getSizeAndUnitTmp" | cut -d ":" -f 1 `
            getUnitSrc=` echo "$getSizeAndUnitTmp" | cut -d ":" -f 2 `

            #
            counterSourceComputed=$(( counterSourceComputed + 1 ))
        fi
        
        #
        if [[ ` cat "$tmp_flagNextOngoingComputed" 2> /dev/null | grep -w -- "true" ` ]]
        then
            #
            echo "false" > "$tmp_flagNextOngoingComputed"

            # Get the size of the ongoing data
            (
                getSizeOfOngoingData=$(get_size "$a_destination_dir_target_embed")

                #
                echo "$getSizeOfOngoingData" > "$tmp_sizeOfOngoingData"
                
                #
                echo "true" > "$tmp_flagNextOngoingComputed"

            ) &
        fi

        #
        a_length_ongoing_data=` cat "$tmp_sizeOfOngoingData" 2> /dev/null | tr -d "[[:space:]]" `


        # Get send percentage
        if [[ $a_size_source -gt 0 ]]
        then
            #
            a_percent_stat_bar=` echo  "( ($a_length_ongoing_data / $a_size_source) * 20 )" | bc -l `
            percent_stat_ongoing=` echo  "( ($a_length_ongoing_data / $a_size_source) * 100 )" | bc -l `

                
            # Take only the part of integer
            a_percent_stat_bar=` echo "$a_percent_stat_bar" | cut -d "." -f1 `
            percent_stat_ongoing=` echo "$percent_stat_ongoing" | cut -d "." -f1 `


            # Treatment on the percent state of the progress bar
            if [[ $a_percent_stat_bar -ge 2 ]]
            then
                #
                a_percent_stat_bar=$(( a_percent_stat_bar - 1 ))
            fi

            # Treatment on the percent state of the ongoing data
            if [[ $percent_stat_ongoing -ge 2  ]]
            then
                #
                percent_stat_ongoing=$(( percent_stat_ongoing - 1 ))
            fi
            
            
            # Update <$a_new_percent>
            a_new_percent=$a_percent_stat_bar


            # Call the function
            getSizeAndUnitTmp=` byteTransformer $a_length_ongoing_data `

            #
            getSizeLinkedUnitOngoing=` echo "$getSizeAndUnitTmp" | cut -d ":" -f 1 `
            getUnitOngoing=` echo "$getSizeAndUnitTmp" | cut -d ":" -f 2 `
        fi 

        
        #
        if [[ $a_new_percent -ge $a_old_percent ]]
        then
            # Remove the content of the line
            #printf "\033[2K\r"


            # Set the color to white then to cyan
            echo -en "\033[37m\r|"
            echo -en "\033[0m\033[1;36m"
            
            # Display the front character according the index and ..
            for counter in `seq 1 $a_percent_stat_bar`
            do
                echo -en "${a_character_bar_front_list[2]}" 
            done

            # Set the color to white
            echo -en "\033[37m"

            #
            decrement_bar_back=$(( 20 - a_percent_stat_bar ))
                
            # Decrement the backward bar
            for counter2 in `seq 1 $decrement_bar_back`
            do
                echo -en "$a_character_bar_back"
            done
            
            #
            printf "| \033[1;036m%d\033[0m" $(( percent_stat_ongoing + 0 ))
            echo -en "\e[1;036m%\e[0m [${getSizeLinkedUnitOngoing}${getUnitOngoing}/${getSizeLinkedUnitSrc}${getUnitSrc}] "

            # Update <$a_old_percent>
            a_old_percent=$a_new_percent
        fi


        #
        if [[ ! ` ps -p "$getPidCommandMv" | grep -w -- "$getPidCommandMv" ` ]]
        then
            # Remove the content of the line
            printf "\033[2K\r"


            #
            a_terminate_process="FALSE"
           
            # Remove the file containing the `pid`
            if [[ -e "$a_filePidCommandMv" ]]
            then
                rm -rf "$a_filePidCommandMv" 2> /dev/null
            fi
            
            #
            if [[ $a_flagSIGTERM == "FALSE" ]]
            then
                # Set the color to white then to green
                echo -en "\033[37m\r|"
                echo -en "\033[0m\033[1;32m"
                
                # Display the front character according the index and ..
                for counter in {1..20}
                do
                    echo -en "${a_character_bar_front_list[2]}"
                done
    
                # Set the color to white
                echo -en "\033[37m"
               
                #
                if [[ $counterSourceComputed -gt 0 ]]
                then
                   #
                    printf "| \033[1;032m%d\033[0m" $(( 5 * 20 ))
                    echo -en "\e[1;032m%\e[0m [${getSizeLinkedUnitSrc}${getUnitSrc}/${getSizeLinkedUnitSrc}${getUnitSrc}]"
                else
                    #
                    printf "| \033[1;032m%d\033[0m" $(( 5 * 20 ))
                    echo -en "\e[1;032m%\e[0m"
                fi
            fi
        fi

        #
        sleep 0.005

    done

    # Call the destructor <<__del__>>
    __del__ 
}


trap setFlagSIGINT SIGINT

# Call the <<display_progress_bar>> method
display_progress_bar $1 $2   
