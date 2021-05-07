export GAZEBO_MASTER_URI=${GAZEBO_MASTER_URI:-http://localhost:11345}
export GAZEBO_MODEL_DATABASE_URI=http://models.gazebosim.org
export GAZEBO_RESOURCE_PATH=/app/share/OGRE/Media
export GAZEBO_PLUGIN_PATH=/app/lib/gazebo-11/plugins:${GAZEBO_PLUGIN_PATH}
export GAZEBO_MODEL_PATH=/app/share/gazebo-11/models:${GAZEBO_MODEL_PATH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/app/lib/gazebo-11/plugins
export OGRE_RESOURCE_PATH=/app/lib/OGRE
