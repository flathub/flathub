"""
Connection Manager for sshPilot
Handles SSH connections, configuration, and secure password storage
"""

import os
import asyncio
import logging
import configparser
import getpass
import subprocess
import shlex
import signal
from typing import Dict, List, Optional, Any, Tuple, Union

import secretstorage
import socket
import time
from gi.repository import GObject, GLib

# Set up asyncio event loop for GTK integration
if os.name == 'posix':
    import gi
    gi.require_version('Gtk', '4.0')
    from gi.repository import Gtk, GLib
    
    # Set up the asyncio event loop
    if not hasattr(GLib, 'MainLoop'):
        import asyncio
        import asyncio.events
        import asyncio.base_events
        import asyncio.unix_events
        
        class GLibEventLoopPolicy(asyncio.events.BaseDefaultEventLoopPolicy):
            _loop_factory = asyncio.SelectorEventLoop
            
            def new_event_loop(self):
                return asyncio.unix_events.DefaultEventLoopPolicy.new_event_loop(self)
        
        asyncio.set_event_loop_policy(GLibEventLoopPolicy())

logger = logging.getLogger(__name__)

class Connection:
    """Represents an SSH connection"""
    
    def __init__(self, data: Dict[str, Any]):
        self.data = data
        self.is_connected = False
        self.connection = None
        self.forwarders: List[asyncio.Task] = []
        self.listeners: List[asyncio.Server] = []
        
        self.nickname = data.get('nickname', data.get('host', 'Unknown'))
        self.host = data.get('host', '')
        self.username = data.get('username', '')
        self.port = data.get('port', 22)
        # previously: self.keyfile = data.get('keyfile', '')
        self.keyfile = data.get('keyfile') or data.get('private_key', '') or ''
        self.password = data.get('password', '')
        self.key_passphrase = data.get('key_passphrase', '')
        # Commands
        self.local_command = data.get('local_command', '')
        self.remote_command = data.get('remote_command', '')
        # Authentication method: 0 = key-based, 1 = password
        try:
            self.auth_method = int(data.get('auth_method', 0))
        except Exception:
            self.auth_method = 0
        # X11 forwarding preference
        self.x11_forwarding = bool(data.get('x11_forwarding', False))
        
        # Key selection mode: 0 try all, 1 specific key
        try:
            self.key_select_mode = int(data.get('key_select_mode', 0) or 0)
        except Exception:
            self.key_select_mode = 0

        # Port forwarding rules
        self.forwarding_rules = data.get('forwarding_rules', [])
        
        # Asyncio event loop
        self.loop = asyncio.get_event_loop()

    def __str__(self):
        return f"{self.nickname} ({self.username}@{self.host})"
        
    async def connect(self):
        """Prepare SSH command for later use (no preflight echo)."""
        try:
            # Build SSH command
            ssh_cmd = ['ssh']

            # Pull advanced SSH defaults from config when available
            try:
                from .config import Config  # avoid circular import at top level
                cfg = Config()
                ssh_cfg = cfg.get_ssh_config()
            except Exception:
                ssh_cfg = {}
            apply_adv = bool(ssh_cfg.get('apply_advanced', False))
            connect_timeout = int(ssh_cfg.get('connection_timeout', 10)) if apply_adv else None
            connection_attempts = int(ssh_cfg.get('connection_attempts', 1)) if apply_adv else None
            strict_host = str(ssh_cfg.get('strict_host_key_checking', '')) if apply_adv else ''
            batch_mode = bool(ssh_cfg.get('batch_mode', False)) if apply_adv else False
            compression = bool(ssh_cfg.get('compression', True)) if apply_adv else False
            verbosity = int(ssh_cfg.get('verbosity', 0))
            debug_enabled = bool(ssh_cfg.get('debug_enabled', False))
            auto_add_host_keys = bool(ssh_cfg.get('auto_add_host_keys', True))

            # Apply advanced args only when user explicitly enabled them
            if apply_adv:
                if batch_mode:
                    ssh_cmd.extend(['-o', 'BatchMode=yes'])
                if connect_timeout is not None:
                    ssh_cmd.extend(['-o', f'ConnectTimeout={connect_timeout}'])
                if connection_attempts is not None:
                    ssh_cmd.extend(['-o', f'ConnectionAttempts={connection_attempts}'])
                if strict_host:
                    ssh_cmd.extend(['-o', f'StrictHostKeyChecking={strict_host}'])
                if compression:
                    ssh_cmd.append('-C')
            ssh_cmd.extend(['-o', 'ExitOnForwardFailure=yes'])

            # Apply default host key behavior when not explicitly set
            try:
                if (not strict_host) and auto_add_host_keys:
                    ssh_cmd.extend(['-o', 'StrictHostKeyChecking=accept-new'])
            except Exception:
                pass

            # Apply verbosity flags
            try:
                v = max(0, min(3, int(verbosity)))
                for _ in range(v):
                    ssh_cmd.append('-v')
                if v == 1:
                    ssh_cmd.extend(['-o', 'LogLevel=VERBOSE'])
                elif v == 2:
                    ssh_cmd.extend(['-o', 'LogLevel=DEBUG2'])
                elif v >= 3:
                    ssh_cmd.extend(['-o', 'LogLevel=DEBUG3'])
                elif debug_enabled:
                    ssh_cmd.extend(['-o', 'LogLevel=DEBUG'])
            except Exception:
                pass
            
            # Add key file only when key-based auth and specific key mode
            try:
                if int(getattr(self, 'auth_method', 0) or 0) == 0 and int(getattr(self, 'key_select_mode', 0) or 0) == 1:
                    if self.keyfile and os.path.exists(self.keyfile):
                        ssh_cmd.extend(['-i', self.keyfile])
                        if self.key_passphrase:
                            logger.warning("Passphrase-protected keys may require additional setup")
            except Exception:
                pass
            
            # Add host and port
            if self.port != 22:
                ssh_cmd.extend(['-p', str(self.port)])
                
            # Add username if specified
            ssh_cmd.append(f"{self.username}@{self.host}" if self.username else self.host)

            # No preflight: store command and mark as ready; real errors will come from spawned ssh
            self.ssh_cmd = ssh_cmd
            self.is_connected = True
            return True
                
        except Exception as e:
            logger.error(f"Failed to connect to {self}: {e}")
            self.is_connected = False
            return False
            
    async def disconnect(self):
        """Close the SSH connection and clean up"""
        if not self.is_connected:
            return
            
        try:
            # Cancel all forwarding tasks
            for task in self.forwarders:
                if not task.done():
                    task.cancel()
            
            # Close all listeners
            for listener in self.listeners:
                listener.close()
            
            # Clean up any running processes
            if hasattr(self, 'process') and self.process:
                try:
                    # Try to terminate gracefully first
                    self.process.terminate()
                    try:
                        # Wait a bit for the process to terminate
                        await asyncio.wait_for(self.process.wait(), timeout=2.0)
                    except asyncio.TimeoutError:
                        # Force kill if it doesn't terminate
                        self.process.kill()
                        await self.process.wait()
                except ProcessLookupError:
                    # Process already terminated
                    pass
                except Exception as e:
                    logger.error(f"Error terminating SSH process: {e}")
                finally:
                    self.process = None
            
            logger.info(f"Disconnected from {self}")
            return True
            
        except Exception as e:
            logger.error(f"Error during disconnect: {e}")
            return False
        finally:
            # Always ensure is_connected is set to False
            self.is_connected = False
            self.listeners.clear()
        
    async def setup_forwarding(self):
        """Set up all forwarding rules"""
        if not self.is_connected or not self.connection:
            return False
            
        success = True
        for rule in self.forwarding_rules:
            if not rule.get('enabled', True):
                continue
                
            rule_type = rule.get('type')
            listen_addr = rule.get('listen_addr', 'localhost')
            listen_port = rule.get('listen_port')
            
            try:
                if rule_type == 'dynamic':
                    # Start SOCKS proxy server
                    await self.start_dynamic_forwarding(listen_addr, listen_port)
                elif rule_type == 'local':
                    # Local port forwarding
                    remote_host = rule.get('remote_host', 'localhost')
                    remote_port = rule.get('remote_port')
                    await self.start_local_forwarding(listen_addr, listen_port, remote_host, remote_port)
                elif rule_type == 'remote':
                    # Remote port forwarding
                    remote_host = rule.get('remote_host', 'localhost')
                    remote_port = rule.get('remote_port')
                    await self.start_remote_forwarding(listen_addr, listen_port, remote_host, remote_port)
                    
            except Exception as e:
                logger.error(f"Failed to set up {rule_type} forwarding: {e}")
                success = False
                
        return success
        
    async def _forward_data(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter, label: str):
        """Helper method to forward data between two streams"""
        try:
            while True:
                data = await reader.read(4096)
                if not data:
                    break
                writer.write(data)
                await writer.drain()
        except (ConnectionError, asyncio.CancelledError):
            pass  # Connection closed
        except Exception as e:
            logger.error(f"Error in {label}: {e}")
        finally:
            writer.close()
            
    async def start_dynamic_forwarding(self, listen_addr: str, listen_port: int):
        """Start dynamic port forwarding (SOCKS proxy) using system SSH client"""
        try:
            logger.debug(f"Starting dynamic port forwarding setup for {self.host} on {listen_addr}:{listen_port}")
            
            # Build the complete SSH command for dynamic port forwarding
            ssh_cmd = ['ssh', '-v']  # Add verbose flag for debugging

            # Read config for options
            try:
                from .config import Config
                cfg = Config()
                ssh_cfg = cfg.get_ssh_config()
            except Exception:
                ssh_cfg = {}
            connect_timeout = int(ssh_cfg.get('connection_timeout', 10))
            connection_attempts = int(ssh_cfg.get('connection_attempts', 1))
            keepalive_interval = int(ssh_cfg.get('keepalive_interval', 30))
            keepalive_count = int(ssh_cfg.get('keepalive_count_max', 3))
            strict_host = str(ssh_cfg.get('strict_host_key_checking', 'accept-new'))
            batch_mode = bool(ssh_cfg.get('batch_mode', True))

            # Robust non-interactive options to prevent hangs
            if batch_mode:
                ssh_cmd.extend(['-o', 'BatchMode=yes'])
            ssh_cmd.extend(['-o', f'ConnectTimeout={connect_timeout}'])
            ssh_cmd.extend(['-o', f'ConnectionAttempts={connection_attempts}'])
            ssh_cmd.extend(['-o', f'ServerAliveInterval={keepalive_interval}'])
            ssh_cmd.extend(['-o', f'ServerAliveCountMax={keepalive_count}'])
            if strict_host:
                ssh_cmd.extend(['-o', f'StrictHostKeyChecking={strict_host}'])
            
            # Add key file if specified
            if self.keyfile and os.path.exists(self.keyfile):
                logger.debug(f"Using SSH key: {self.keyfile}")
                ssh_cmd.extend(['-i', self.keyfile])
                if self.key_passphrase:
                    logger.debug("Key has a passphrase")
            else:
                logger.debug("No SSH key specified or key not found")
                
            # Add host and port
            if self.port != 22:
                logger.debug(f"Using custom SSH port: {self.port}")
                ssh_cmd.extend(['-p', str(self.port)])
                
            # Add dynamic port forwarding option
            forward_spec = f"{listen_addr}:{listen_port}"
            logger.debug(f"Setting up dynamic forwarding to: {forward_spec}")
            
            ssh_cmd.extend([
                '-N',  # No remote command
                '-D', forward_spec,  # Dynamic port forwarding (SOCKS)
                '-f',  # Run in background
                '-o', 'ExitOnForwardFailure=yes',  # Exit if forwarding fails
                '-o', 'ServerAliveInterval=30',    # Keep connection alive
                '-o', 'ServerAliveCountMax=3'      # Max missed keepalives before disconnect
            ])
            
            # Add username and host
            target = f"{self.username}@{self.host}" if self.username else self.host
            ssh_cmd.append(target)
            
            # Log the full command (without sensitive data)
            logger.debug(f"SSH command: {' '.join(ssh_cmd[:10])}...")
            
            # Start the SSH process
            logger.info(f"Starting dynamic port forwarding with command: {' '.join(ssh_cmd)}")
            self.process = await asyncio.create_subprocess_exec(
                *ssh_cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            
            # Wait a bit to catch any immediate errors
            try:
                stdout, stderr = await asyncio.wait_for(self.process.communicate(), timeout=5.0)
                if stdout:
                    logger.debug(f"SSH stdout: {stdout.decode().strip()}")
                if stderr:
                    logger.debug(f"SSH stderr: {stderr.decode().strip()}")
                    
                if self.process.returncode != 0:
                    error_msg = stderr.decode().strip() if stderr else "Unknown error"
                    logger.error(f"SSH dynamic port forwarding failed with code {self.process.returncode}: {error_msg}")
                    raise Exception(f"SSH dynamic port forwarding failed: {error_msg}")
                else:
                    logger.info("SSH process started successfully")
            except asyncio.TimeoutError:
                # If we get here, the process is still running which is good
                logger.debug("SSH process is running in background")
                
                # Check if the port is actually listening
                try:
                    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                        s.settimeout(1)
                        result = s.connect_ex((listen_addr, int(listen_port)))
                        if result == 0:
                            logger.info(f"Successfully verified port {listen_port} is listening")
                        else:
                            logger.warning(f"Port {listen_port} is not listening (connect result: {result})")
                except Exception as e:
                    logger.warning(f"Could not verify if port is listening: {e}")
            
            logger.info(f"Dynamic port forwarding (SOCKS) started on {listen_addr}:{listen_port}")
            
            # Store the forwarding rule
            rule = {
                'type': 'dynamic',
                'listen_addr': listen_addr,
                'listen_port': listen_port,
                'process': self.process,
                'start_time': time.time()
            }
            self.forwarding_rules.append(rule)
            logger.debug(f"Added forwarding rule: {rule}")
            
            # Log all forwarding rules for debugging
            logger.debug(f"Current forwarding rules: {self.forwarding_rules}")
            
            return True
            
        except Exception as e:
            logger.error(f"Dynamic port forwarding failed: {e}", exc_info=True)
            if hasattr(self, 'process') and self.process:
                try:
                    logger.debug("Terminating SSH process due to error")
                    self.process.terminate()
                    await asyncio.wait_for(self.process.wait(), timeout=2.0)
                except (ProcessLookupError, asyncio.TimeoutError) as e:
                    logger.debug(f"Error terminating process: {e}")
                    pass
            raise

    async def start_local_forwarding(self, listen_addr: str, listen_port: int, remote_host: str, remote_port: int):
        """Start local port forwarding using system SSH client"""
        try:
            # Build the SSH command for local port forwarding
            ssh_cmd = self.ssh_cmd + [
                '-N',  # No remote command
                '-L', f"{listen_addr}:{listen_port}:{remote_host}:{remote_port}"
            ]
            
            # Start the SSH process
            self.process = await asyncio.create_subprocess_exec(
                *ssh_cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            
            # Check if the process started successfully
            if self.process.returncode is not None and self.process.returncode != 0:
                stderr = await self.process.stderr.read()
                raise Exception(f"SSH port forwarding failed: {stderr.decode().strip()}")
            
            logger.info(f"Local forwarding started: {listen_addr}:{listen_port} -> {remote_host}:{remote_port}")
            
            # Store the forwarding rule
            self.forwarding_rules.append({
                'type': 'local',
                'listen_addr': listen_addr,
                'listen_port': listen_port,
                'remote_host': remote_host,
                'remote_port': remote_port,
                'process': self.process
            })
            
            # Wait for the process to complete
            await self.process.wait()
            
        except Exception as e:
            logger.error(f"Local forwarding failed: {e}")
            if hasattr(self, 'process') and self.process:
                self.process.terminate()
                await self.process.wait()
            raise

class ConnectionManager(GObject.Object):
    """Manages SSH connections and configuration"""
    
    __gsignals__ = {
        'connection-added': (GObject.SignalFlags.RUN_FIRST, None, (object,)),
        'connection-removed': (GObject.SignalFlags.RUN_FIRST, None, (object,)),
        'connection-updated': (GObject.SignalFlags.RUN_FIRST, None, (object,)),
        'connection-status-changed': (GObject.SignalFlags.RUN_FIRST, None, (object, bool)),
    }

    def __init__(self):
        super().__init__()
        self.connections: List[Connection] = []
        self.ssh_config = {}
        self.ssh_config_path = os.path.expanduser('~/.ssh/config')
        self.known_hosts_path = os.path.expanduser('~/.ssh/known_hosts')
        self.loop = asyncio.get_event_loop()
        self.active_connections: Dict[str, asyncio.Task] = {}
        
        # Load SSH config immediately for fast UI
        self.load_ssh_config()

        # Defer slower operations to idle to avoid blocking startup
        GLib.idle_add(self._post_init_slow_path)

    def _post_init_slow_path(self):
        """Run slower initialization steps after UI is responsive."""
        try:
            # Key scan
            self.load_ssh_keys()
        except Exception as e:
            logger.debug(f"SSH key scan skipped/failed: {e}")
        
        # Initialize secure storage (can be slow)
        try:
            self.bus = secretstorage.dbus_init()
            self.collection = secretstorage.get_default_collection(self.bus)
            logger.info("Secure storage initialized")
        except Exception as e:
            logger.warning(f"Failed to initialize secure storage: {e}")
            self.collection = None
        return False  # run once

    def _ensure_collection(self) -> bool:
        """Ensure secretstorage collection is initialized and unlocked."""
        try:
            if getattr(self, 'collection', None) is None:
                try:
                    self.bus = secretstorage.dbus_init()
                    self.collection = secretstorage.get_default_collection(self.bus)
                except Exception as e:
                    logger.warning(f"Secret storage init failed: {e}")
                    self.collection = None
                    return False
            # Attempt to unlock if locked
            try:
                if hasattr(self.collection, 'is_locked') and self.collection.is_locked():
                    unlocked, _ = self.collection.unlock()
                    if not unlocked:
                        logger.warning("Secret storage collection remains locked")
                        return False
            except Exception as e:
                logger.debug(f"Could not unlock collection: {e}")
            return self.collection is not None
        except Exception:
            return False
        
    def load_ssh_config(self):
        """Load connections from SSH config file"""
        try:
            # Reset current list to reflect latest config on each load
            self.connections = []
            if not os.path.exists(self.ssh_config_path):
                logger.info("SSH config file not found, creating empty one")
                os.makedirs(os.path.dirname(self.ssh_config_path), exist_ok=True)
                with open(self.ssh_config_path, 'w') as f:
                    f.write("# SSH configuration file\n")
                return

            # Simple SSH config parser
            current_host = None
            current_config = {}
            
            with open(self.ssh_config_path, 'r') as f:
                for line in f:
                    line = line.strip()
                    if not line or line.startswith('#'):
                        continue
                        
                    if ' ' in line:
                        key, value = line.split(maxsplit=1)
                        key = key.lower()
                        # Preserve quotes in value; we'll handle key-specific unquoting later
                        value = value
                        
                        if key == 'host':
                            # Save previous host if exists
                            if current_host and current_config:
                                connection_data = self.parse_host_config(current_config)
                                if connection_data:
                                    # Build connection and apply per-connection metadata (e.g., auth_method)
                                    conn = Connection(connection_data)
                                    try:
                                        from .config import Config
                                        cfg = Config()
                                        meta = cfg.get_connection_meta(conn.nickname)
                                        if isinstance(meta, dict) and 'auth_method' in meta:
                                            conn.auth_method = meta['auth_method']
                                    except Exception:
                                        pass
                                    self.connections.append(conn)
                            
                            # Start new host
                            current_host = value
                            current_config = {'host': value}
                        else:
                            # Handle multiple values for the same key (like multiple LocalForward rules)
                            if key in current_config and key in ['localforward', 'remoteforward', 'dynamicforward']:
                                if not isinstance(current_config[key], list):
                                    current_config[key] = [current_config[key]]
                                current_config[key].append(value)
                            else:
                                current_config[key] = value
            
            # Add the last host
            if current_host and current_config:
                connection_data = self.parse_host_config(current_config)
                if connection_data:
                    conn = Connection(connection_data)
                    # Apply per-connection metadata from app config (auth method, etc.)
                    try:
                        from .config import Config
                        cfg = Config()
                        meta = cfg.get_connection_meta(conn.nickname)
                        if isinstance(meta, dict):
                            if 'auth_method' in meta:
                                conn.auth_method = meta['auth_method']
                    except Exception:
                        pass
                    self.connections.append(conn)
            
            logger.info(f"Loaded {len(self.connections)} connections from SSH config")
            
        except Exception as e:
            logger.error(f"Failed to load SSH config: {e}", exc_info=True)

    def parse_host_config(self, config: Dict[str, Any]) -> Optional[Dict[str, Any]]:
        """Parse host configuration from SSH config"""
        try:
            def _unwrap(val: Any) -> Any:
                if isinstance(val, str) and len(val) >= 2:
                    if (val.startswith('"') and val.endswith('"')) or (val.startswith("'") and val.endswith("'")):
                        return val[1:-1]
                return val

            host = _unwrap(config.get('host', ''))
            if not host:
                return None
                
            # Extract relevant configuration
            parsed = {
                'nickname': host,
                'host': _unwrap(config.get('hostname', host)),
                'port': int(_unwrap(config.get('port', 22))),
                'username': _unwrap(config.get('user', getpass.getuser())),
                # previously: 'private_key': config.get('identityfile'),
                'keyfile': os.path.expanduser(_unwrap(config.get('identityfile'))) if config.get('identityfile') else None,
                'forwarding_rules': []
            }
            # Map ForwardX11 yes/no → x11_forwarding boolean
            try:
                fwd_x11 = str(config.get('forwardx11', 'no')).strip().lower()
                parsed['x11_forwarding'] = fwd_x11 in ('yes', 'true', '1', 'on')
            except Exception:
                parsed['x11_forwarding'] = False
            
            # Handle port forwarding rules
            for forward_type in ['localforward', 'remoteforward', 'dynamicforward']:
                if forward_type not in config:
                    continue
                    
                forward_specs = config[forward_type]
                if not isinstance(forward_specs, list):
                    forward_specs = [forward_specs]
                    
                for forward_spec in forward_specs:
                    if forward_type == 'dynamicforward':
                        # Format is usually "[bind_address:]port"
                        if ':' in forward_spec:
                            bind_addr, port_str = forward_spec.rsplit(':', 1)
                            listen_port = int(port_str)
                        else:
                            bind_addr = '127.0.0.1'  # Default bind address
                            listen_port = int(forward_spec)
                        
                        parsed['forwarding_rules'].append({
                            'type': 'dynamic',
                            'listen_addr': bind_addr,
                            'listen_port': listen_port,
                            'enabled': True
                        })
                    else:
                        # Handle LocalForward and RemoteForward
                        # Format is "[bind_address:]port host:hostport"
                        parts = forward_spec.split()
                        if len(parts) == 2:
                            listen_spec, dest_spec = parts
                            
                            # Parse listen address and port
                            if ':' in listen_spec:
                                bind_addr, port_str = listen_spec.rsplit(':', 1)
                                listen_port = int(port_str)
                            else:
                                bind_addr = '127.0.0.1'  # Default bind address
                                listen_port = int(listen_spec)
                            
                            # Parse destination host and port
                            if ':' in dest_spec:
                                remote_host, remote_port = dest_spec.split(':')
                                remote_port = int(remote_port)
                            else:
                                remote_host = dest_spec
                                remote_port = 22  # Default SSH port
                            
                            rule_type = 'local' if forward_type == 'localforward' else 'remote'
                            if rule_type == 'local':
                                parsed['forwarding_rules'].append({
                                    'type': 'local',
                                    'listen_addr': bind_addr,
                                    'listen_port': listen_port,
                                    'remote_host': remote_host,
                                    'remote_port': remote_port,
                                    'enabled': True
                                })
                            else:
                                # RemoteForward: remote host/port listens, destination is local host/port
                                parsed['forwarding_rules'].append({
                                    'type': 'remote',
                                    'listen_addr': bind_addr,   # remote host
                                    'listen_port': listen_port, # remote port
                                    'local_host': remote_host,  # destination host (local)
                                    'local_port': remote_port,  # destination port (local)
                                    'enabled': True
                                })
            
            # Handle proxy settings if any
            if 'proxycommand' in config:
                parsed['proxy_command'] = config['proxycommand']
            
            # Commands: LocalCommand requires PermitLocalCommand
            try:
                def _unescape_cfg_value(val: str) -> str:
                    if not isinstance(val, str):
                        return val
                    v = val.strip()
                    # If the value is wrapped in double quotes, strip only the outer quotes
                    if len(v) >= 2 and v.startswith('"') and v.endswith('"'):
                        v = v[1:-1]
                    # Convert escaped quotes back for UI
                    v = v.replace('\\"', '"').replace('\\\\', '\\')
                    return v

                if 'localcommand' in config:
                    parsed['local_command'] = _unescape_cfg_value(config.get('localcommand', ''))
                if 'remotecommand' in config:
                    parsed['remote_command'] = _unescape_cfg_value(config.get('remotecommand', ''))
                # Map RequestTTY to a boolean flag to aid terminal decisions if needed
                if 'requesttty' in config:
                    parsed['request_tty'] = str(config.get('requesttty', '')).strip().lower() in ('yes', 'force', 'true', '1', 'on')
            except Exception:
                pass

            # Key selection mode: if IdentitiesOnly is set truthy, select specific key
            try:
                ident_only = str(config.get('identitiesonly', '')).strip().lower()
                if ident_only in ('yes', 'true', '1', 'on'):
                    parsed['key_select_mode'] = 1
                else:
                    parsed['key_select_mode'] = 0
            except Exception:
                parsed['key_select_mode'] = 0
                
            return parsed
            
        except Exception as e:
            logger.error(f"Error parsing host config: {e}", exc_info=True)
            return None

    def load_ssh_keys(self):
        """Auto-detect SSH keys in ~/.ssh/"""
        ssh_dir = os.path.expanduser('~/.ssh')
        if not os.path.exists(ssh_dir):
            return
        
        try:
            keys = []
            for filename in os.listdir(ssh_dir):
                if filename.endswith('.pub'):
                    private_key = os.path.join(ssh_dir, filename[:-4])
                    if os.path.exists(private_key):
                        keys.append(private_key)
            
            logger.info(f"Found {len(keys)} SSH keys: {keys}")
            return keys
            
        except Exception as e:
            logger.error(f"Failed to load SSH keys: {e}")
            return []

    def store_password(self, host: str, username: str, password: str):
        """Store password securely in system keyring"""
        if not self._ensure_collection():
            logger.warning("Secure storage not available, password not stored")
            return False
        
        try:
            attributes = {
                'application': 'sshPilot',
                'host': host,
                'username': username
            }
            
            # Delete existing password if any
            existing_items = list(self.collection.search_items(attributes))
            for item in existing_items:
                item.delete()
            
            # Store new password
            self.collection.create_item(
                f'sshPilot: {username}@{host}',
                attributes,
                password.encode()
            )
            
            logger.debug(f"Password stored for {username}@{host}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to store password: {e}")
            return False

    def get_password(self, host: str, username: str) -> Optional[str]:
        """Retrieve password from system keyring"""
        if not self._ensure_collection():
            return None
        
        try:
            attributes = {
                'application': 'sshPilot',
                'host': host,
                'username': username
            }
            
            items = list(self.collection.search_items(attributes))
            if items:
                password = items[0].get_secret().decode()
                logger.debug(f"Password retrieved for {username}@{host}")
                return password
            return None
            
        except Exception as e:
            logger.error(f"Error retrieving password for {username}@{host}: {e}")
            return None

    def delete_password(self, host: str, username: str) -> bool:
        """Delete stored password for host/user from system keyring"""
        if not self._ensure_collection():
            return False
        try:
            attributes = {
                'application': 'sshPilot',
                'host': host,
                'username': username
            }
            items = list(self.collection.search_items(attributes))
            removed_any = False
            for item in items:
                try:
                    item.delete()
                    removed_any = True
                except Exception:
                    pass
            if removed_any:
                logger.debug(f"Deleted stored password for {username}@{host}")
            return removed_any
        except Exception as e:
            logger.error(f"Error deleting password for {username}@{host}: {e}")
            return False

    def format_ssh_config_entry(self, data: Dict[str, Any]) -> str:
        """Format connection data as SSH config entry"""
        lines = [f"Host {data['nickname']}"]
        
        # Add basic connection info
        lines.append(f"    HostName {data.get('host', '')}")
        lines.append(f"    User {data.get('username', '')}")
        
        # Add port if specified and not default
        port = data.get('port')
        if port and port != 22:  # Only add port if it's not the default 22
            lines.append(f"    Port {port}")
        
        # Add IdentityFile/IdentitiesOnly per selection when auth is key-based
        keyfile = data.get('keyfile') or data.get('private_key')
        auth_method = int(data.get('auth_method', 0) or 0)
        key_select_mode = int(data.get('key_select_mode', 0) or 0)  # 0=try all, 1=specific
        if auth_method == 0:
            # Always write IdentityFile if a concrete key path is provided (even in mode 0)
            if keyfile and keyfile.strip() and not keyfile.strip().lower().startswith('select key file'):
                if ' ' in keyfile and not (keyfile.startswith('"') and keyfile.endswith('"')):
                    keyfile = f'"{keyfile}"'
                lines.append(f"    IdentityFile {keyfile}")
            # Only enforce exclusive key usage in mode 1
            if key_select_mode == 1:
                lines.append("    IdentitiesOnly yes")
        
        # Add X11 forwarding if enabled
        if data.get('x11_forwarding', False):
            lines.append("    ForwardX11 yes")

        # Add LocalCommand if specified, ensure PermitLocalCommand (write exactly as provided)
        local_cmd = (data.get('local_command') or '').strip()
        if local_cmd:
            lines.append("    PermitLocalCommand yes")
            lines.append(f"    LocalCommand {local_cmd}")

        # Add RemoteCommand and RequestTTY if specified (ensure shell stays active)
        remote_cmd = (data.get('remote_command') or '').strip()
        if remote_cmd:
            # Ensure we keep an interactive shell after the command
            remote_cmd_aug = remote_cmd if 'exec $SHELL' in remote_cmd else f"{remote_cmd} ; exec $SHELL -l"
            # Write RemoteCommand first, then RequestTTY (order for readability)
            lines.append(f"    RemoteCommand {remote_cmd_aug}")
            lines.append("    RequestTTY yes")
        
        # Add port forwarding rules if any (ensure sane defaults)
        for rule in data.get('forwarding_rules', []):
            listen_addr = rule.get('listen_addr', '') or '127.0.0.1'
            listen_port = rule.get('listen_port', '')
            if not listen_port:
                continue
            listen_spec = f"{listen_addr}:{listen_port}"
            
            if rule.get('type') == 'local':
                dest_spec = f"{rule.get('remote_host', '')}:{rule.get('remote_port', '')}"
                lines.append(f"    LocalForward {listen_spec} {dest_spec}")
            elif rule.get('type') == 'remote':
                # For RemoteForward we forward remote listen -> local destination
                dest_spec = f"{rule.get('local_host') or rule.get('remote_host', '')}:{rule.get('local_port') or rule.get('remote_port', '')}"
                lines.append(f"    RemoteForward {listen_spec} {dest_spec}")
            elif rule.get('type') == 'dynamic':
                lines.append(f"    DynamicForward {listen_spec}")
        
        return '\n'.join(lines)

    def update_ssh_config_file(self, connection: Connection, new_data: Dict[str, Any]):
        """Update SSH config file with new connection data"""
        try:
            if not os.path.exists(self.ssh_config_path):
                # If config file doesn't exist, create it with the new connection
                os.makedirs(os.path.dirname(self.ssh_config_path), exist_ok=True)
                with open(self.ssh_config_path, 'w') as f:
                    f.write("# SSH configuration file\n")
                
                # Add the new connection
                with open(self.ssh_config_path, 'a') as f:
                    updated_config = self.format_ssh_config_entry(new_data)
                    f.write('\n' + updated_config + '\n')
                return
            
            # Read current config
            try:
                with open(self.ssh_config_path, 'r') as f:
                    lines = f.readlines()
            except IOError as e:
                logger.error(f"Failed to read SSH config: {e}")
                raise
            
            # Find and update the connection's Host block (alias-aware and indentation-robust)
            updated_lines = []
            old_name = str(getattr(connection, 'nickname', '') or '')
            new_name = str(new_data.get('nickname') or old_name)
            host_found = False
            replaced_once = False

            # Only consider exact full-value matches or exact alias token matches; do not split the
            # nickname into tokens for matching, to avoid cross-matching on common words (e.g. "host").
            candidate_names = {old_name, new_name}

            i = 0
            while i < len(lines):
                raw_line = lines[i]
                lstripped = raw_line.lstrip()

                if lstripped.startswith('Host '):
                    full_value = lstripped[len('Host '):].strip()
                    parts = lstripped.split()
                    current_names = parts[1:] if len(parts) > 1 else []
                else:
                    full_value = ''
                    current_names = []

                if full_value or current_names:
                    if (full_value in candidate_names) or any(name in candidate_names for name in current_names):
                        host_found = True
                        if not replaced_once:
                            updated_config = self.format_ssh_config_entry(new_data)
                            updated_lines.append(updated_config + '\n')
                            replaced_once = True
                        i += 1
                        while i < len(lines) and not lines[i].lstrip().startswith('Host '):
                            i += 1
                        continue

                updated_lines.append(raw_line)
                i += 1
            
            # If host not found, append the new config (new or old name not present)
            if not host_found:
                updated_config = self.format_ssh_config_entry(new_data)
                updated_lines.append('\n' + updated_config + '\n')
            
            # Write the updated config back to file
            try:
                with open(self.ssh_config_path, 'w') as f:
                    f.writelines(updated_lines)
                logger.info(
                    "Wrote SSH config for host %s (found=%s, rules=%d) to %s",
                    new_name,
                    host_found,
                    len(new_data.get('forwarding_rules', []) or []),
                    self.ssh_config_path,
                )
            except IOError as e:
                logger.error(f"Failed to write SSH config: {e}")
                raise
        except Exception as e:
            logger.error(f"Error updating SSH config: {e}", exc_info=True)
            raise

    def remove_ssh_config_entry(self, host_nickname: str) -> bool:
        """Remove a Host block from ~/.ssh/config by nickname.

        Returns True if a block was removed, False if not found or on error.
        """
        try:
            if not os.path.exists(self.ssh_config_path):
                return False
            try:
                with open(self.ssh_config_path, 'r') as f:
                    lines = f.readlines()
            except IOError as e:
                logger.error(f"Failed to read SSH config for delete: {e}")
                return False

            updated_lines = []
            i = 0
            removed = False
            # Alias-aware and indentation-robust deletion
            # Only match exact full value or exact alias token equal to the nickname
            candidate_names = {host_nickname}

            while i < len(lines):
                raw_line = lines[i]
                lstripped = raw_line.lstrip()
                if lstripped.startswith('Host '):
                    full_value = lstripped[len('Host '):].strip()
                    parts = lstripped.split()
                    current_names = parts[1:] if len(parts) > 1 else []
                    if (full_value in candidate_names) or any(name in candidate_names for name in current_names):
                        removed = True
                        i += 1
                        while i < len(lines) and not lines[i].lstrip().startswith('Host '):
                            i += 1
                        continue
                # Keep line
                updated_lines.append(raw_line)
                i += 1

            if removed:
                try:
                    with open(self.ssh_config_path, 'w') as f:
                        f.writelines(updated_lines)
                except IOError as e:
                    logger.error(f"Failed to write SSH config after delete: {e}")
                    return False
            return removed
        except Exception as e:
            logger.error(f"Error removing SSH config entry: {e}", exc_info=True)
            return False

    def update_connection(self, connection: Connection, new_data: Dict[str, Any]) -> bool:
        """Update an existing connection"""
        try:
            logger.info(
                "Updating connection '%s' → writing to %s (rules=%d)",
                connection.nickname,
                self.ssh_config_path,
                len(new_data.get('forwarding_rules', []) or [])
            )
            # Capture previous identifiers for credential cleanup
            prev_host = getattr(connection, 'host', '')
            prev_user = getattr(connection, 'username', '')
            # Update connection data
            connection.data.update(new_data)
            # Ensure forwarding rules stored on the object are updated too
            try:
                connection.forwarding_rules = list(new_data.get('forwarding_rules', connection.forwarding_rules or []))
            except Exception:
                pass
            
            # Update the SSH config file
            self.update_ssh_config_file(connection, new_data)
            
            # Handle password storage/removal
            if 'password' in new_data:
                pwd = new_data.get('password') or ''
                # Determine current identifiers after update
                curr_host = new_data.get('host') or getattr(connection, 'host', prev_host)
                curr_user = new_data.get('username') or getattr(connection, 'username', prev_user)
                if pwd:
                    self.store_password(curr_host, curr_user, pwd)
                else:
                    # Remove any stored passwords for both previous and current identifiers
                    try:
                        if prev_host and prev_user:
                            self.delete_password(prev_host, prev_user)
                    except Exception:
                        pass
                    try:
                        if curr_host and curr_user and (curr_host != prev_host or curr_user != prev_user):
                            self.delete_password(curr_host, curr_user)
                    except Exception:
                        pass
            
            # Reload SSH config to reflect changes
            self.load_ssh_config()
            # Update the provided connection object from the freshly loaded list to ensure runtime uses the latest
            try:
                fresh = self.find_connection_by_nickname(new_data.get('nickname', connection.nickname))
                if fresh:
                    # Copy runtime-critical fields (auth/key selection) so active sessions use latest
                    connection.auth_method = getattr(fresh, 'auth_method', connection.auth_method)
                    connection.keyfile = getattr(fresh, 'keyfile', connection.keyfile)
                    connection.key_select_mode = getattr(fresh, 'key_select_mode', getattr(connection, 'key_select_mode', 0))
            except Exception:
                pass
            
            # Emit signal
            self.emit('connection-updated', connection)
            
            logger.info(f"Connection updated: {connection}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to update connection: {e}")
            return False

    def remove_connection(self, connection: Connection) -> bool:
        """Remove connection from config and list"""
        try:
            # Remove from list
            if connection in self.connections:
                self.connections.remove(connection)
            
            # Remove password from keyring
            if self.collection:
                try:
                    attributes = {
                        'application': 'sshPilot',
                        'host': connection.host,
                        'username': connection.username
                    }
                    items = list(self.collection.search_items(attributes))
                    for item in items:
                        item.delete()
                except Exception as e:
                    logger.warning(f"Failed to remove password from keyring: {e}")
            
            # Remove from SSH config file
            try:
                removed = self.remove_ssh_config_entry(connection.nickname)
                logger.debug(f"SSH config entry removed={removed} for {connection.nickname}")
            except Exception as e:
                logger.warning(f"Failed to remove SSH config entry for {connection.nickname}: {e}")
            
            # Remove per-connection metadata (auth method, etc.) to avoid lingering entries
            try:
                from .config import Config
                cfg = Config()
                meta_all = cfg.get_setting('connections_meta', {}) or {}
                if isinstance(meta_all, dict) and connection.nickname in meta_all:
                    del meta_all[connection.nickname]
                    cfg.set_setting('connections_meta', meta_all)
                    logger.debug(f"Removed metadata for {connection.nickname}")
            except Exception as e:
                logger.debug(f"Could not remove metadata for {connection.nickname}: {e}")
            
            # Emit signal
            self.emit('connection-removed', connection)
            
            # Reload connections so in-memory list reflects latest file state
            try:
                self.load_ssh_config()
            except Exception:
                pass

            logger.info(f"Connection removed: {connection}")
            return True
            
        except Exception as e:
            logger.error(f"Failed to remove connection: {e}")
            return False

    async def connect(self, connection: Connection):
        """Connect to an SSH host asynchronously"""
        try:
            # Connect to the SSH server
            connected = await connection.connect()
            if not connected:
                raise Exception("Failed to establish SSH connection")
            
            # Set up port forwarding if needed
            if connection.forwarding_rules:
                await connection.setup_forwarding()
            
            # Store the connection task
            if connection.host in self.active_connections:
                self.active_connections[connection.host].cancel()
            
            # Create a task to keep the connection alive
            async def keepalive():
                try:
                    while connection.is_connected:
                        try:
                            # Send keepalive every 30 seconds
                            await asyncio.sleep(30)
                            if connection.connection and connection.is_connected:
                                await connection.connection.ping()
                        except (ConnectionError, asyncio.CancelledError):
                            break
                        except Exception as e:
                            logger.error(f"Keepalive error for {connection}: {e}")
                            break
                finally:
                    if connection.is_connected:
                        await connection.disconnect()
                    connection.is_connected = False
                    self.emit('connection-status-changed', connection, False)
                    logger.info(f"Disconnected from {connection}")
            
            # Start the keepalive task
            task = asyncio.create_task(keepalive())
            self.active_connections[connection.host] = task
            
            # Update the connection state and emit status change
            connection.is_connected = True
            GLib.idle_add(self.emit, 'connection-status-changed', connection, True)
            logger.info(f"Connected to {connection}")
            
            return True
            
        except Exception as e:
            error_msg = f"Failed to connect to {connection}: {e}"
            logger.error(error_msg, exc_info=True)
            if hasattr(connection, 'connection') and connection.connection:
                await connection.disconnect()
            connection.is_connected = False
            raise Exception(error_msg) from e
    
    async def disconnect(self, connection: Connection):
        """Disconnect from SSH host and clean up resources asynchronously"""
        try:
            # Cancel the keepalive task if it exists
            if connection.host in self.active_connections:
                self.active_connections[connection.host].cancel()
                try:
                    await self.active_connections[connection.host]
                except asyncio.CancelledError:
                    pass
                del self.active_connections[connection.host]
            
            # Disconnect the connection
            if hasattr(connection, 'connection') and connection.connection and connection.is_connected:
                await connection.disconnect()
            
            # Update the connection state and emit status change signal
            connection.is_connected = False
            GLib.idle_add(self.emit, 'connection-status-changed', connection, False)
            logger.info(f"Disconnected from {connection}")
            
        except Exception as e:
            logger.error(f"Failed to disconnect from {connection}: {e}", exc_info=True)
            raise

    def get_connections(self) -> List[Connection]:
        """Get list of all connections"""
        return self.connections.copy()

    def find_connection_by_nickname(self, nickname: str) -> Optional[Connection]:
        """Find connection by nickname"""
        for connection in self.connections:
            if connection.nickname == nickname:
                return connection
        return None