import os
import logging
import gc
import psutil
import time
from urllib.parse import urlparse
import requests
from pathlib import Path
from typing import Optional, Dict, Any, List, Tuple
from functools import lru_cache
from wselector.models import WallpaperInfo

logger = logging.getLogger(__name__)

# Memory management configuration
MAX_CACHE_SIZE_MB = 100  # Maximum allowed cache size in MB
MEMORY_CHECK_INTERVAL = 30  # Check memory every 30 seconds
LAST_MEMORY_CHECK = 0

def get_memory_usage() -> Tuple[float, float]:
    """Get current memory usage in MB and percentage."""
    process = psutil.Process()
    mem_info = process.memory_info()
    return mem_info.rss / (1024 * 1024), process.memory_percent()

def log_memory_usage(prefix: str = ""):
    """Log current memory usage with an optional prefix."""
    mb_used, percent = get_memory_usage()
    logging.info(f"{prefix}Memory usage: {mb_used:.2f}MB ({percent:.1f}%)")
    return mb_used

def check_memory_usage():
    """Check if memory usage is too high and perform cleanup if needed."""
    global LAST_MEMORY_CHECK
    
    current_time = time.time()
    if current_time - LAST_MEMORY_CHECK < MEMORY_CHECK_INTERVAL:
        return
        
    LAST_MEMORY_CHECK = current_time
    
    mb_used, _ = get_memory_usage()
    if mb_used > MAX_CACHE_SIZE_MB:
        logging.warning(f"Memory usage high: {mb_used:.2f}MB, performing cleanup...")
        
        # Clear Python's garbage collector
        collected = gc.collect()
        logging.info(f"Garbage collector collected {collected} objects")
        
        # Clear function caches
        clear_caches()
        
        # Log new memory usage
        new_mb = log_memory_usage("After cleanup: ")
        if new_mb > MAX_CACHE_SIZE_MB * 0.9:  # Still too high
            logging.warning("Memory still high after cleanup, consider increasing MAX_CACHE_SIZE_MB")

def clear_caches():
    """Clear various caches to free memory."""
    # Clear Python's function caches
    for obj in gc.get_objects():
        if callable(obj) and hasattr(obj, '__code__'):
            # Only try to clear cache if the function has the attribute
            if hasattr(obj, 'cache_clear') and callable(getattr(obj, 'cache_clear', None)):
                try:
                    obj.cache_clear()
                except Exception as e:
                    logging.debug(f"Could not clear cache for {obj.__name__}: {e}")
    
    # Clear other known caches
    import functools
    if hasattr(functools, '_lru_cache_wrapper') and hasattr(functools._lru_cache_wrapper, 'cache_clear'):
        try:
            functools._lru_cache_wrapper.cache_clear()
        except Exception as e:
            logging.debug(f"Could not clear lru_cache wrapper: {e}")
    
    # Clear requests session cache
    try:
        session = requests.Session()
        session.cache.clear()
    except Exception:
        pass

@lru_cache(maxsize=1024)
def get_cached_thumbnail_path(url: str, cache_dir: str) -> Optional[str]:
    """Get or download a thumbnail with caching."""
    try:
        # Create cache directory if it doesn't exist
        os.makedirs(cache_dir, exist_ok=True)
        
        # Get filename from URL
        filename = os.path.basename(urlparse(url).path)
        if not filename:
            filename = f"thumbnail_{hash(url)}.jpg"
            
        cache_path = os.path.join(cache_dir, filename)
        if os.path.exists(cache_path):
            return cache_path
            
        # Download the image with timeout
        response = requests.get(url, stream=True, timeout=10)
        response.raise_for_status()
        
        # Save the image in chunks
        with open(cache_path, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                if not chunk:  # Filter out keep-alive chunks
                    continue
                f.write(chunk)
        
        # Check cache size and clean up if needed
        cleanup_cache(cache_dir, max_size_mb=MAX_CACHE_SIZE_MB)
        
        return cache_path
        
    except Exception as e:
        logging.error(f"Failed to download thumbnail {url}: {e}")
        return None

def cleanup_cache(cache_dir: str, max_size_mb: int):
    """Clean up cache directory if it exceeds max_size_mb."""
    try:
        if not os.path.exists(cache_dir):
            return
            
        # Get all files with their access times and sizes
        files = []
        total_size = 0
        
        for root, _, filenames in os.walk(cache_dir):
            for filename in filenames:
                filepath = os.path.join(root, filename)
                try:
                    stat = os.stat(filepath)
                    files.append((filepath, stat.st_atime, stat.st_size))
                    total_size += stat.st_size
                except OSError:
                    continue
        
        # Convert to MB
        total_size_mb = total_size / (1024 * 1024)
        
        # If under limit, do nothing
        if total_size_mb <= max_size_mb:
            return
            
        # Sort files by access time (oldest first)
        files.sort(key=lambda x: x[1])
        
        # Remove oldest files until under limit
        for filepath, _, size in files:
            if total_size_mb <= max_size_mb * 0.9:  # Stop at 90% of max size
                break
                
            try:
                os.remove(filepath)
                total_size_mb -= size / (1024 * 1024)
            except OSError:
                continue
                
    except Exception as e:
        logging.error(f"Error cleaning up cache: {e}")

def download_thumbnail(wp: WallpaperInfo) -> str:
    """
    Download and cache a wallpaper thumbnail.
    
    Args:
        wp: WallpaperInfo object containing thumbnail URL and ID
        
    Returns:
        str: Path to the downloaded thumbnail file
    """
    try:
        check_memory_usage()
        cache_dir = os.path.dirname(wp.get_cached_path())
        return get_cached_thumbnail_path(wp.thumbnail_url, cache_dir)
        
    except Exception as e:
        logger.error(f"Failed to download thumbnail for {wp.id}: {e}")
        raise

def download_thumbnail_old(wp: WallpaperInfo) -> str:
    """
    Download and cache a wallpaper thumbnail.
    
    Args:
        wp: WallpaperInfo object containing thumbnail URL and ID
        
    Returns:
        str: Path to the downloaded thumbnail file
    """
    try:
        # Ensure cache directory exists
        os.makedirs(os.path.dirname(wp.get_cached_path()), exist_ok=True)
        
        # Download the thumbnail
        response = requests.get(wp.thumbnail_url, timeout=10, stream=True)
        response.raise_for_status()
        
        # Save to cache
        temp_path = f"{wp.get_cached_path()}.tmp"
        with open(temp_path, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)
        
        # Rename temp file to final name (atomic operation)
        final_path = wp.get_cached_path()
        os.replace(temp_path, final_path)
        
        logger.debug(f"Cached thumbnail: {wp.id} at {final_path}")
        return final_path
        
    except requests.RequestException as e:
        logger.error(f"Failed to download thumbnail for {wp.id}: {e}")
        raise
    except OSError as e:
        logger.error(f"Failed to save thumbnail for {wp.id}: {e}")
        raise
    except Exception as e:
        logger.error(f"Unexpected error processing thumbnail for {wp.id}: {e}")
        raise
