import logging
import requests
from typing import List, Optional
from wselector.models import WallpaperInfo

# Setup logger
logger = logging.getLogger(__name__)

class WSelectorScraper:
    """Handles interaction with the Wallhaven API for WSelector."""
    
    BASE_URL = "https://wallhaven.cc/api/v1"
    
    def search_wallpapers(self, query: Optional[str] = None, categories: str = "111", 
                         purity: str = "100", page: int = 1, sort: str = "latest",
                         resolution: Optional[str] = None) -> tuple[list[WallpaperInfo], bool]:
        """Search for wallpapers on Wallhaven.
        
        Args:
            query: Search query string (will be URL-encoded)
            categories: String of 3 digits (0/1) for General/Anime/People categories
            purity: String of 3 digits (0/1) for SFW/Sketchy/NSFW content
            page: Page number to fetch
            sort: Sorting method ('latest', 'popular', 'random', 'views', 'favorites')
            resolution: Optional resolution filter (e.g., '1920x1080'). Uses 'atleast' parameter to find wallpapers that are at least this size.
            
        Returns:
            A tuple containing (wallpapers, has_next_page) where:
            - wallpapers: List of WallpaperInfo objects
            - has_next_page: Boolean indicating if there are more pages available
        """
        # Map UI sort options to Wallhaven API sort parameters
        sort_mapping = {
            'latest': 'date_added',
            'popular': 'toplist',
            'random': 'random',
            'views': 'views',
            'favorites': 'favorites',
            'relevance': 'relevance'
        }
        
        # If there's a search query and sort is 'popular', use 'relevance' instead
        if query and sort.lower() == 'popular':
            logger.info("Search query detected with 'popular' sort, using 'relevance' instead")
            sort_param = 'relevance'
        else:
            # Default to 'date_added' if sort mode is not recognized
            sort_param = sort_mapping.get(sort.lower(), 'date_added')
        
        # Initialize params with common parameters
        params = {
            'categories': categories,
            'purity': purity,
            'sorting': sort_param,
            'order': 'desc' if sort_param != 'random' else 'asc',
            'page': str(page)  # Convert page number to string for the API
        }
        
        # Add search query if provided (it will be properly URL-encoded by requests)
        if query and query.strip():
            params['q'] = query.strip()
            
        # Add resolution filter if provided
        if resolution:
            # Use 'atleast' parameter instead of 'resolutions' to get wallpapers that are at least this size
            # This gives more results than the exact resolution match
            params['atleast'] = resolution
        
        try:
            # Create a custom ordered params dictionary to put query first
            ordered_params = {}
            
            # Add query first if it exists
            if 'q' in params:
                ordered_params['q'] = params['q']
                
            # Add all other parameters
            for key, value in params.items():
                if key != 'q':
                    ordered_params[key] = value
            
            # Log the full URL with parameters in the desired order
            from urllib.parse import urlencode
            full_url = f"{self.BASE_URL}/search?{urlencode(ordered_params, doseq=True)}"
            logger.info(f"Making API request to: {full_url}")
            
            # Use the ordered params for the actual request
            response = requests.get(f"{self.BASE_URL}/search", params=ordered_params)
            response.raise_for_status()
            data = response.json()

            # Log basic response info
            logger.info(f"API response status: {response.status_code}")
            
            # Get pagination info
            meta = data.get('meta', {})
            current_page = meta.get('current_page', 1)
            last_page = meta.get('last_page', 1)
            has_next_page = current_page < last_page
            
            logger.info(f"Page {current_page} of {last_page}, has_next_page: {has_next_page}")
            
            wallpapers = []
            for wp in data.get('data', []):
                wallpaper = WallpaperInfo(
                    id=wp.get('id', ''),
                    url=wp.get('path', ''),
                    thumbnail_url=wp.get('thumbs', {}).get('small', ''),
                    title=wp.get('id', 'Untitled'),
                    category=wp.get('category', ''),
                    purity=wp.get('purity', '')
                )
                # Log each wallpaper's details
                logger.debug(f"Found wallpaper - ID: {wallpaper.id}, URL: {wallpaper.url}")
                wallpapers.append(wallpaper)
            
            # Log total number of wallpapers found
            logger.info(f"Found {len(wallpapers)} wallpapers in response")
            
            # If no wallpapers found, log a message
            if not wallpapers:
                logger.info("No wallpapers found matching the criteria")
                
            return wallpapers, has_next_page
            
        except requests.RequestException as e:
            logger.error(f"Error fetching wallpapers: {e}", exc_info=True)
            return [], False
