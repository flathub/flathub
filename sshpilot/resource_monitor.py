"""
Resource Monitor for sshPilot
Monitors system resources on remote SSH hosts
"""

import re
import time
import logging
import threading
from collections import deque
from typing import Dict, List, Optional, Tuple, Any

import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.backends.backend_gtk4agg import FigureCanvasGTK4Agg as FigureCanvas
from matplotlib.figure import Figure

import gi
gi.require_version('Gtk', '4.0')
from gi.repository import Gtk, GLib, GObject

import paramiko

logger = logging.getLogger(__name__)

class ResourceData:
    """Container for resource monitoring data"""
    
    def __init__(self, max_points: int = 300):
        self.max_points = max_points
        self.timestamps = deque(maxlen=max_points)
        self.cpu_usage = deque(maxlen=max_points)
        self.memory_usage = deque(maxlen=max_points)
        self.memory_total = deque(maxlen=max_points)
        self.disk_usage = deque(maxlen=max_points)
        self.disk_total = deque(maxlen=max_points)
        self.network_rx = deque(maxlen=max_points)
        self.network_tx = deque(maxlen=max_points)
        self.load_avg = deque(maxlen=max_points)

    def add_data_point(self, data: Dict[str, Any]):
        """Add a new data point"""
        self.timestamps.append(time.time())
        self.cpu_usage.append(data.get('cpu_usage', 0))
        self.memory_usage.append(data.get('memory_usage', 0))
        self.memory_total.append(data.get('memory_total', 0))
        self.disk_usage.append(data.get('disk_usage', 0))
        self.disk_total.append(data.get('disk_total', 0))
        self.network_rx.append(data.get('network_rx', 0))
        self.network_tx.append(data.get('network_tx', 0))
        self.load_avg.append(data.get('load_avg', 0))

    def get_latest(self) -> Dict[str, Any]:
        """Get the latest data point"""
        if not self.timestamps:
            return {}
        
        return {
            'timestamp': self.timestamps[-1],
            'cpu_usage': self.cpu_usage[-1],
            'memory_usage': self.memory_usage[-1],
            'memory_total': self.memory_total[-1],
            'disk_usage': self.disk_usage[-1],
            'disk_total': self.disk_total[-1],
            'network_rx': self.network_rx[-1],
            'network_tx': self.network_tx[-1],
            'load_avg': self.load_avg[-1],
        }

class ResourceMonitor(GObject.Object):
    """Resource monitoring manager"""
    
    __gsignals__ = {
        'data-updated': (GObject.SignalFlags.RUN_FIRST, None, (object, object)),
        'monitoring-started': (GObject.SignalFlags.RUN_FIRST, None, (object,)),
        'monitoring-stopped': (GObject.SignalFlags.RUN_FIRST, None, (object,)),
        'error-occurred': (GObject.SignalFlags.RUN_FIRST, None, (object, str)),
    }
    
    def __init__(self, config=None):
        super().__init__()
        self.config = config
        self.monitoring_sessions = {}  # connection -> (thread, resource_data)
        self.update_interval = 5  # seconds
        
        if config:
            monitoring_config = config.get_monitoring_config()
            self.update_interval = monitoring_config.get('update_interval', 5)

    def start_monitoring(self, connection) -> bool:
        """Start monitoring a connection"""
        if not connection or not connection.is_connected:
            logger.error("Cannot monitor disconnected connection")
            return False
        
        if connection in self.monitoring_sessions:
            logger.warning(f"Already monitoring {connection}")
            return True
        
        try:
            # Create resource data container
            max_points = 300
            if self.config:
                monitoring_config = self.config.get_monitoring_config()
                max_points = monitoring_config.get('history_length', 300)
            
            resource_data = ResourceData(max_points)
            
            # Start monitoring thread
            stop_event = threading.Event()
            monitor_thread = threading.Thread(
                target=self._monitoring_thread,
                args=(connection, resource_data, stop_event),
                daemon=True
            )
            
            self.monitoring_sessions[connection] = {
                'thread': monitor_thread,
                'resource_data': resource_data,
                'stop_event': stop_event
            }
            
            monitor_thread.start()
            
            # Emit signal
            self.emit('monitoring-started', connection)
            
            logger.info(f"Started monitoring {connection}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to start monitoring {connection}: {e}")
            return False

    def stop_monitoring(self, connection):
        """Stop monitoring a connection"""
        if connection not in self.monitoring_sessions:
            return
        
        try:
            session = self.monitoring_sessions[connection]
            session['stop_event'].set()
            
            # Wait for thread to finish (with timeout)
            session['thread'].join(timeout=2.0)
            
            del self.monitoring_sessions[connection]
            
            # Emit signal
            self.emit('monitoring-stopped', connection)
            
            logger.info(f"Stopped monitoring {connection}")
            
        except Exception as e:
            logger.error(f"Failed to stop monitoring {connection}: {e}")

    def get_resource_data(self, connection) -> Optional[ResourceData]:
        """Get resource data for a connection"""
        if connection in self.monitoring_sessions:
            return self.monitoring_sessions[connection]['resource_data']
        return None

    def is_monitoring(self, connection) -> bool:
        """Check if a connection is being monitored"""
        return connection in self.monitoring_sessions

    def _monitoring_thread(self, connection, resource_data: ResourceData, stop_event: threading.Event):
        """Monitoring thread function"""
        logger.debug(f"Monitoring thread started for {connection}")
        
        # Keep track of previous network stats for rate calculation
        prev_network_stats = None
        prev_timestamp = None
        transport_fail_count = 0
        MAX_TRANSPORT_FAILS = 3  # Maximum number of consecutive transport failures before giving up
        
        while not stop_event.wait(self.update_interval):
            try:
                if not connection.is_connected or not connection.client:
                    logger.debug(f"Connection lost for {connection}")
                    break
                
                # First check basic connection state
                if not connection.is_connected:
                    logger.warning("Connection is marked as disconnected")
                    break
                    
                # Then check transport state
                try:
                    transport = connection.client.get_transport()
                    if not transport or not transport.is_active():
                        transport_fail_count += 1
                        logger.warning(f"SSH transport check failed ({transport_fail_count}/{MAX_TRANSPORT_FAILS})")
                        if transport_fail_count >= MAX_TRANSPORT_FAILS:
                            logger.error("Maximum transport failures reached, stopping monitoring")
                            break
                        # Exponential backoff delay (1s, 2s, 4s)
                        delay = 2 ** (transport_fail_count - 1)
                        logger.debug(f"Waiting {delay}s before retry")
                        time.sleep(delay)
                        continue
                        
                    # Reset counter and enable keepalive on successful check
                    if transport_fail_count > 0:
                        transport.set_keepalive(60)  # Re-enable keepalive
                        logger.info("Transport recovered, resetting failure count")
                    transport_fail_count = 0
                        
                except Exception as e:
                    logger.warning(f"Transport check error: {e}")
                    transport_fail_count += 1
                    if transport_fail_count >= MAX_TRANSPORT_FAILS:
                        logger.error("Maximum transport failures reached, stopping monitoring")
                        break
                    # Use same exponential backoff for exceptions
                    delay = 2 ** (transport_fail_count - 1)
                    logger.debug(f"Waiting {delay}s before retry after error")
                    time.sleep(delay)
                    continue
                
                # Fetch system information
                data = self._fetch_system_info(connection)
                
                if data:
                    # Calculate network rates if we have previous data
                    if prev_network_stats and prev_timestamp:
                        time_diff = time.time() - prev_timestamp
                        if time_diff > 0:
                            rx_rate = (data['network_rx_bytes'] - prev_network_stats['rx_bytes']) / time_diff
                            tx_rate = (data['network_tx_bytes'] - prev_network_stats['tx_bytes']) / time_diff
                            data['network_rx'] = max(0, rx_rate)  # bytes per second
                            data['network_tx'] = max(0, tx_rate)  # bytes per second
                    
                    # Store current network stats for next iteration
                    prev_network_stats = {
                        'rx_bytes': data.get('network_rx_bytes', 0),
                        'tx_bytes': data.get('network_tx_bytes', 0)
                    }
                    prev_timestamp = time.time()
                    
                    # Add data point
                    resource_data.add_data_point(data)
                    
                    # Emit signal on main thread
                    GLib.idle_add(self.emit, 'data-updated', connection, resource_data)
                
            except Exception as e:
                logger.error(f"Monitoring error for {connection}: {e}")
                GLib.idle_add(self.emit, 'error-occurred', connection, str(e))
                break
        
        logger.debug(f"Monitoring thread stopped for {connection}")

    def _fetch_system_info(self, connection, retry_count=0) -> Optional[Dict[str, Any]]:
        """Fetch system information via SSH"""
        MAX_RETRIES = 2  # Maximum number of refresh attempts
        
        try:
            client = connection.client
            if not client or not connection.is_connected:
                logger.warning("SSH client is not connected")
                return None
            
            try:
                # Test the connection first and attempt to fix if needed
                transport = client.get_transport()
                if not transport or not transport.is_active():
                    if retry_count >= MAX_RETRIES:
                        logger.warning("Maximum connection refresh attempts reached")
                        return None
                        
                    logger.warning(f"SSH transport is not active, attempting to refresh (attempt {retry_count + 1}/{MAX_RETRIES})")
                    try:
                        if hasattr(connection, 'refresh_connection') and connection.refresh_connection():
                            logger.info("Successfully refreshed SSH connection")
                            # Small delay to allow connection to stabilize
                            time.sleep(0.5)
                            return self._fetch_system_info(connection, retry_count + 1)
                            
                        # Only try keepalive if refresh_connection is not available or failed
                        transport = client.get_transport()
                        if transport:
                            transport.set_keepalive(60)  # Enable keepalive every 60 seconds
                            transport.send_ignore()  # Send keepalive packet
                            if transport.is_active():
                                logger.info("Successfully refreshed SSH transport with keepalive")
                                return self._fetch_system_info(connection, retry_count + 1)
                    except Exception as e:
                        logger.warning(f"Could not refresh SSH transport: {e}")
                        return None
            except Exception as e:
                logger.warning(f"SSH transport check failed: {e}")
                return None
            
            # Commands to get system information
            commands = {
                'cpu': "cat /proc/stat | head -n1",
                'memory': "cat /proc/meminfo",
                'disk': "df -h /",
                'network': "cat /proc/net/dev",
                'loadavg': "cat /proc/loadavg",
                'uptime': "cat /proc/uptime"
            }
            
            results = {}
            for key, cmd in commands.items():
                try:
                    # Set a timeout for the command execution
                    stdin, stdout, stderr = client.exec_command(cmd, timeout=5)
                    
                    # Read with timeout
                    output = stdout.read().decode().strip()
                    error = stderr.read().decode().strip()
                    
                    if error:
                        logger.warning(f"Command '{cmd}' error: {error}")
                    
                    # Check the exit status
                    if stdout.channel.recv_exit_status() != 0:
                        logger.warning(f"Command '{cmd}' failed with non-zero exit status")
                        continue
                    
                    results[key] = output
                except Exception as e:
                    logger.warning(f"Failed to execute command '{cmd}': {e}")
                    continue
            
            # Parse the results
            return self._parse_system_info(results)
            
        except Exception as e:
            logger.error(f"Failed to fetch system info: {e}")
            return None

    def _parse_system_info(self, results: Dict[str, str]) -> Dict[str, Any]:
        """Parse system information from command outputs"""
        data = {}
        
        try:
            # Parse CPU usage from /proc/stat
            cpu_line = results.get('cpu', '')
            if cpu_line:
                cpu_times = [int(x) for x in cpu_line.split()[1:8]]
                idle_time = cpu_times[3]  # idle time
                total_time = sum(cpu_times)
                if total_time > 0:
                    data['cpu_usage'] = (total_time - idle_time) / total_time * 100
                else:
                    data['cpu_usage'] = 0
            
            # Parse memory usage from /proc/meminfo
            meminfo = results.get('memory', '')
            if meminfo:
                mem_total = 0
                mem_available = 0
                
                for line in meminfo.split('\n'):
                    if line.startswith('MemTotal:'):
                        mem_total = int(line.split()[1]) * 1024  # Convert KB to bytes
                    elif line.startswith('MemAvailable:'):
                        mem_available = int(line.split()[1]) * 1024  # Convert KB to bytes
                
                if mem_total > 0:
                    data['memory_total'] = mem_total
                    data['memory_usage'] = mem_total - mem_available
                else:
                    data['memory_total'] = 0
                    data['memory_usage'] = 0
            
            # Parse disk usage from df
            disk_output = results.get('disk', '')
            if disk_output:
                lines = disk_output.strip().split('\n')
                if len(lines) >= 2:
                    # Parse the data line (skip header)
                    parts = lines[1].split()
                    if len(parts) >= 6:
                        try:
                            # df output: Filesystem Size Used Avail Use% Mounted
                            size_str = parts[1]
                            used_str = parts[2]
                            
                            # Convert to bytes (df shows in 1K blocks by default)
                            data['disk_total'] = self._parse_size_string(size_str) * 1024
                            data['disk_usage'] = self._parse_size_string(used_str) * 1024
                        except (ValueError, IndexError):
                            data['disk_total'] = 0
                            data['disk_usage'] = 0
            
            # Parse network statistics from /proc/net/dev
            network_output = results.get('network', '')
            if network_output:
                total_rx_bytes = 0
                total_tx_bytes = 0
                
                for line in network_output.split('\n')[2:]:  # Skip header lines
                    if ':' in line:
                        parts = line.split()
                        if len(parts) >= 10:
                            interface = parts[0].rstrip(':')
                            # Skip loopback interface
                            if interface != 'lo':
                                try:
                                    rx_bytes = int(parts[1])
                                    tx_bytes = int(parts[9])
                                    total_rx_bytes += rx_bytes
                                    total_tx_bytes += tx_bytes
                                except ValueError:
                                    continue
                
                data['network_rx_bytes'] = total_rx_bytes
                data['network_tx_bytes'] = total_tx_bytes
            
            # Parse load average
            loadavg_output = results.get('loadavg', '')
            if loadavg_output:
                parts = loadavg_output.split()
                if parts:
                    try:
                        data['load_avg'] = float(parts[0])  # 1-minute load average
                    except ValueError:
                        data['load_avg'] = 0
            
            return data
            
        except Exception as e:
            logger.error(f"Failed to parse system info: {e}")
            return {}

    def _parse_size_string(self, size_str: str) -> int:
        """Parse size string (e.g., '1.5G', '256M') to bytes"""
        try:
            size_str = size_str.upper()
            multipliers = {
                'K': 1024,
                'M': 1024 ** 2,
                'G': 1024 ** 3,
                'T': 1024 ** 4,
            }
            
            if size_str[-1] in multipliers:
                return int(float(size_str[:-1]) * multipliers[size_str[-1]])
            else:
                return int(size_str)
                
        except (ValueError, IndexError):
            return 0

class ResourceView(Gtk.Box):
    """Widget for displaying resource monitoring charts"""
    
    def __init__(self, resource_monitor: ResourceMonitor, connection):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        
        self.resource_monitor = resource_monitor
        self.connection = connection
        self.resource_data = None
        
        # Create header
        self.create_header()
        
        # Create matplotlib figure
        self.figure = Figure(figsize=(12, 8), dpi=100)
        self.figure.patch.set_facecolor('white')
        
        # Create subplots
        self.ax_cpu = self.figure.add_subplot(2, 2, 1)
        self.ax_memory = self.figure.add_subplot(2, 2, 2)
        self.ax_disk = self.figure.add_subplot(2, 2, 3)
        self.ax_network = self.figure.add_subplot(2, 2, 4)
        
        # Configure subplots
        self.setup_plots()
        
        # Create canvas
        self.canvas = FigureCanvas(self.figure)
        self.canvas.set_size_request(800, 600)
        
        # Add canvas to scrolled window
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        scrolled.set_child(self.canvas)
        
        self.append(scrolled)
        
        # Connect to resource monitor signals
        self.resource_monitor.connect('data-updated', self.on_data_updated)
        
        # Start monitoring if not already started
        if not self.resource_monitor.is_monitoring(connection):
            self.resource_monitor.start_monitoring(connection)
        
        # Get existing data
        self.resource_data = self.resource_monitor.get_resource_data(connection)
        
        # Start update timer
        self.update_timer = GLib.timeout_add_seconds(1, self.update_charts)

    def create_header(self):
        """Create header with connection info and controls"""
        header = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        header.add_css_class('toolbar')
        
        # Connection info
        info_label = Gtk.Label()
        info_label.set_markup(f"<b>Resource Monitor</b> - {self.connection}")
        info_label.set_halign(Gtk.Align.START)
        header.append(info_label)
        
        # Spacer
        spacer = Gtk.Box()
        spacer.set_hexpand(True)
        header.append(spacer)
        
        # Control buttons
        refresh_button = Gtk.Button.new_from_icon_name('view-refresh-symbolic')
        refresh_button.set_tooltip_text('Refresh')
        refresh_button.connect('clicked', self.on_refresh_clicked)
        header.append(refresh_button)
        
        self.append(header)

    def setup_plots(self):
        """Set up matplotlib subplots"""
        # CPU usage plot
        self.ax_cpu.set_title('CPU Usage (%)')
        self.ax_cpu.set_ylim(0, 100)
        self.ax_cpu.grid(True, alpha=0.3)
        self.cpu_line, = self.ax_cpu.plot([], [], 'b-', linewidth=2)
        
        # Memory usage plot
        self.ax_memory.set_title('Memory Usage')
        self.ax_memory.grid(True, alpha=0.3)
        self.memory_line, = self.ax_memory.plot([], [], 'g-', linewidth=2, label='Used')
        self.ax_memory.legend()
        
        # Disk usage plot
        self.ax_disk.set_title('Disk Usage')
        self.ax_disk.grid(True, alpha=0.3)
        self.disk_line, = self.ax_disk.plot([], [], 'r-', linewidth=2)
        
        # Network usage plot
        self.ax_network.set_title('Network I/O (bytes/sec)')
        self.ax_network.grid(True, alpha=0.3)
        self.network_rx_line, = self.ax_network.plot([], [], 'c-', linewidth=2, label='RX')
        self.network_tx_line, = self.ax_network.plot([], [], 'm-', linewidth=2, label='TX')
        self.ax_network.legend()
        
        # Adjust layout
        self.figure.tight_layout()

    def on_data_updated(self, monitor, connection, resource_data):
        """Handle data update signal"""
        if connection == self.connection:
            self.resource_data = resource_data

    def update_charts(self):
        """Update charts with latest data"""
        if not self.resource_data or not self.resource_data.timestamps:
            return True
        
        try:
            # Get time range for x-axis (last 5 minutes)
            current_time = time.time()
            time_range = 300  # 5 minutes in seconds
            
            # Filter data to show only last 5 minutes
            timestamps = list(self.resource_data.timestamps)
            cpu_data = list(self.resource_data.cpu_usage)
            memory_data = list(self.resource_data.memory_usage)
            memory_total = list(self.resource_data.memory_total)
            disk_data = list(self.resource_data.disk_usage)
            disk_total = list(self.resource_data.disk_total)
            network_rx = list(self.resource_data.network_rx)
            network_tx = list(self.resource_data.network_tx)
            
            # Filter to time range
            filtered_indices = [i for i, t in enumerate(timestamps) if current_time - t <= time_range]
            
            if not filtered_indices:
                return True
            
            x_data = [timestamps[i] - current_time for i in filtered_indices]  # Relative time
            
            # Update CPU plot
            cpu_y = [cpu_data[i] for i in filtered_indices]
            self.cpu_line.set_data(x_data, cpu_y)
            self.ax_cpu.set_xlim(min(x_data), max(x_data))
            self.ax_cpu.set_ylim(0, max(100, max(cpu_y) * 1.1) if cpu_y else 100)
            
            # Update memory plot
            memory_y = [memory_data[i] / (1024**3) for i in filtered_indices]  # Convert to GB
            self.memory_line.set_data(x_data, memory_y)
            self.ax_memory.set_xlim(min(x_data), max(x_data))
            if memory_y:
                max_memory = max([memory_total[i] / (1024**3) for i in filtered_indices])
                self.ax_memory.set_ylim(0, max_memory * 1.1)
                self.ax_memory.set_ylabel('Memory (GB)')
            
            # Update disk plot (as percentage)
            disk_y = []
            for i in filtered_indices:
                if disk_total[i] > 0:
                    disk_y.append(disk_data[i] / disk_total[i] * 100)
                else:
                    disk_y.append(0)
            
            self.disk_line.set_data(x_data, disk_y)
            self.ax_disk.set_xlim(min(x_data), max(x_data))
            self.ax_disk.set_ylim(0, 100)
            self.ax_disk.set_ylabel('Disk Usage (%)')
            
            # Update network plot
            network_rx_y = [network_rx[i] / (1024**2) for i in filtered_indices]  # Convert to MB/s
            network_tx_y = [network_tx[i] / (1024**2) for i in filtered_indices]  # Convert to MB/s
            
            self.network_rx_line.set_data(x_data, network_rx_y)
            self.network_tx_line.set_data(x_data, network_tx_y)
            self.ax_network.set_xlim(min(x_data), max(x_data))
            
            if network_rx_y or network_tx_y:
                max_network = max(max(network_rx_y) if network_rx_y else 0, 
                                max(network_tx_y) if network_tx_y else 0)
                self.ax_network.set_ylim(0, max_network * 1.1)
                self.ax_network.set_ylabel('Network (MB/s)')
            
            # Redraw canvas
            self.canvas.draw()
            
        except Exception as e:
            logger.error(f"Failed to update charts: {e}")
        
        return True  # Continue timer

    def on_refresh_clicked(self, button):
        """Handle refresh button click"""
        # Force immediate update
        self.update_charts()

    def cleanup(self):
        """Clean up resources"""
        if hasattr(self, 'update_timer'):
            GLib.source_remove(self.update_timer)