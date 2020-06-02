#!/bin/sh

INSTALL_PATH="/app/extra/CloudStationBackup"
LIB_PATH="$INSTALL_PATH/lib"
BIN_PATH="$INSTALL_PATH/bin/launcher"

check_process()
{
	pid=$1
	kill -0 $pid > /dev/null 2>&1
	if [ $? -eq 0 ]; then
		return 1
	else
		return 0
	fi
}

stop_process()
{
	counter=0
	pid=$1
	kill $pid


	while [ 1 ] ; do
		check_process $pid

		if [ $? -eq 0 ]; then
			# already stop
			break;
		fi

		if [ $counter -eq 10 ]; then
			# force kill
			kill -9 $pid
			break;
		fi

		sleep 1
		counter=$((counter+1))
	done
}

gracefull_stop_service()
{
	user_name=$1
	home_path=`eval echo ~"$user_name"`
	app_path="$home_path/.CloudStationBackup"
	pid_file="$app_path/daemon.pid"

	if [ ! -f "$pid_file" ]; then
		#not running
        return 0
	fi

	pid=`grep pid "$pid_file" | sed 's/pid=//g' | sed 's/"//g'`
	stop_process $pid
}

start()
{
user_name=$1
        home_path=`eval echo ~"$user_name"`
        app_path="$home_path/.CloudStationBackup"
        pid_file="$app_path/daemon.pid"

        if [ ! -f "$pid_file" ]; then
                # running
        env "LD_LIBRARY_PATH=$LIB_PATH" "$BIN_PATH" &
        return 0
	fi
}

stop()
{
	gracefull_stop_service $USER
}
