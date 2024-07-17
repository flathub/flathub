use lazy_static::lazy_static;
use log::info;
use std::sync::Arc;
use tokio::sync::Mutex;
use tokio::time::{Duration};
#[cfg(not(any(target_os = "android", target_os = "ios")))]
use std::fs;
#[cfg(not(any(target_os = "android", target_os = "ios")))]
use std::io::Read;
#[cfg(not(any(target_os = "android", target_os = "ios")))]
use crate::config::Config;

use crate::config::Config2;

const API_URI: &'static str = "https://api.hoptodesk.com/                                                                                                                                                                              ";


#[derive(Debug, Clone)]
pub struct ApiError(String);

impl<E: std::error::Error> From<E> for ApiError {
    fn from(e: E) -> Self {
        Self(e.to_string())
    }
}

#[derive(Default)]
struct OnceAPI {
    response: Arc<Mutex<Option<serde_json::Value>>>,
}

impl OnceAPI {
    async fn call(&self) -> Result<serde_json::Value, ApiError> {
        let mut r = self.response.lock().await;
        if let Some(r) = &*r {
            return Ok(r.clone());
        }

        #[cfg(not(any(target_os = "android", target_os = "ios")))]
        {
            let local_api_json = Config::path("api.json");
            //info!("Checking for local api.json: {}", local_api_json.display());
            if let Ok(mut file) = fs::File::open(&local_api_json) {
                let mut body = String::new();
                file.read_to_string(&mut body).ok();
                let ret: serde_json::Value = serde_json::from_str(&body)?;
                *r = Some(ret.clone());
                info!("Loaded local api.json");
                return Ok(ret);
            }
        }
		let api_uri_trim = API_URI.trim();
        let api_uri = Config2::get().options.get("custom-api-url").map(ToOwned::to_owned).unwrap_or_else(|| api_uri_trim.to_owned());
        info!("Loading API {}", api_uri);
        let body = reqwest::get(api_uri).await?.text().await?;
        let ret: serde_json::Value = serde_json::from_str(&body)?;

        let response = self.response.clone(); // make a clone of the Arc<Mutex>
        tokio::spawn(async move {
            loop {
                tokio::time::sleep(Duration::from_secs(30000)).await;
                let api_uri = Config2::get().options.get("custom-api-url").map(ToOwned::to_owned).unwrap_or_else(|| api_uri_trim.to_owned());
                info!("Refreshing API {}", api_uri);
                let body = reqwest::get(api_uri).await;
                match body {
                    Ok(resp) => {
                        let body = resp.text().await;
                        match body {
                            Ok(txt) => {
                                let ret: serde_json::Value = serde_json::from_str(&txt).unwrap_or_else(|e| {
                                    info!("Failed to parse response from API: {}", e);
                                    serde_json::Value::Null
                                });
                                let mut r = response.lock().await;
                                *r = Some(ret.clone());
                            }
                            Err(e) => {
                                info!("Failed to read response from API: {}", e);
                            }
                        }
                    }
                    Err(e) => {
                        info!("Failed to call API: {}", e);
                    }
                }
            }
        });

        *r = Some(ret.clone());
        Ok(ret)
    }

    async fn erase(&self) {
        let mut r = self.response.lock().await;
        *r = None
    }
}


lazy_static! {
    static ref ONCE: OnceAPI = OnceAPI::default();
}

pub async fn call_api() -> Result<serde_json::Value, ApiError> {
    (*ONCE).call().await
}

pub async fn erase_api() {
    (*ONCE).erase().await
}