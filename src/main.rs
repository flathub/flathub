// main.rs

use reqwest::multipart;
use serde::{Deserialize, Serialize};
use serde_json::json;
use std::fs::File;
use std::io::BufRead;
use std::io::{self, BufReader, Read, Write};
use std::path::Path;

#[derive(Deserialize, Serialize)]
struct Credentials {
    client_id: String,
    client_secret: String,
}

#[derive(Deserialize)]
struct DeviceCodeResponse {
    device_code: String,
    user_code: String,
    expires_in: u32,
    interval: u32,
    verification_url: String,
}

#[derive(Deserialize)]
struct TokenResponse {
    access_token: String,
    expires_in: u32,
    refresh_token: Option<String>,
    scope: String,
    token_type: String,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let credentials = load_or_create_credentials()?;

    // Step 1: Get device code
    let device_code_response = get_device_code(&credentials.client_id).await?;
    println!(
        "Go to {} and enter the code: {}",
        device_code_response.verification_url, device_code_response.user_code
    );

    // Wait for user to authenticate...
    println!("Press Enter after authenticating...");
    let mut input = String::new();
    std::io::stdin().read_line(&mut input)?;

    // Solicitar el nombre del archivo .zip
    let file_path = loop {
        println!("Por favor, ingresa el nombre del archivo .zip a subir:");

        let mut file_name = String::new();
        io::stdin().read_line(&mut file_name)?;
        let file_name = file_name.trim();

        // Verificar que el archivo tenga la extensión .zip
        if file_name.ends_with(".zip") {
            break file_name.to_string();
        } else {
            println!("Error: El archivo debe tener la extensión .zip.");
        }
    };

    // Step 2: Exchange device code for access token
    let token_response = get_access_token(
        &credentials.client_id,
        &credentials.client_secret,
        &device_code_response.device_code,
    )
    .await?;
    println!("Access token received: {}", token_response.access_token);

    // Step 3: Upload the file
    let upload_response = upload_file(&token_response.access_token, &file_path).await?;
    println!("File uploaded successfully: {}", upload_response);

    Ok(())
}

fn load_or_create_credentials() -> Result<Credentials, Box<dyn std::error::Error>> {
    let credentials_path = dirs::config_dir().unwrap().join("g-up/credentials.txt");

    if !Path::new(&credentials_path).exists() {
        println!("No se encontró el archivo de credenciales. Creando 'credentials.txt'...");

        if let Some(parent) = credentials_path.parent() {
            std::fs::create_dir_all(parent)?;
        }

        println!("Por favor, ingresa tu client_id:");
        let mut client_id = String::new();
        io::stdin().read_line(&mut client_id)?;

        println!("Por favor, ingresa tu client_secret:");
        let mut client_secret = String::new();
        io::stdin().read_line(&mut client_secret)?;

        let credentials = Credentials {
            client_id: client_id.trim().to_string(),
            client_secret: client_secret.trim().to_string(),
        };

        // Guardar las credenciales en el archivo
        let mut output_file = File::create(&credentials_path)?;
        writeln!(output_file, "client_id={}", credentials.client_id)?;
        writeln!(output_file, "client_secret={}", credentials.client_secret)?;

        println!("Credenciales guardadas en 'credentials.txt'. Vuelve a ejecutar el programa.");
        std::process::exit(0);
    }

    // Leer credenciales del archivo
    let file = File::open(&credentials_path)?;
    let reader = BufReader::new(file);
    let mut client_id = String::new();
    let mut client_secret = String::new();

    for line in reader.lines() {
        let line = line?;
        if line.starts_with("client_id=") {
            client_id = line["client_id=".len()..].to_string();
        } else if line.starts_with("client_secret=") {
            client_secret = line["client_secret=".len()..].to_string();
        }
    }

    Ok(Credentials {
        client_id: client_id.trim().to_string(),
        client_secret: client_secret.trim().to_string(),
    })
}

async fn get_device_code(client_id: &str) -> Result<DeviceCodeResponse, reqwest::Error> {
    let client = reqwest::Client::new();
    let params = [
        ("client_id", client_id),
        ("scope", "https://www.googleapis.com/auth/drive.file"),
    ];

    let res = client
        .post("https://oauth2.googleapis.com/device/code")
        .form(&params)
        .send()
        .await?
        .json::<DeviceCodeResponse>()
        .await?;

    Ok(res)
}

async fn get_access_token(
    client_id: &str,
    client_secret: &str,
    device_code: &str,
) -> Result<TokenResponse, reqwest::Error> {
    let client = reqwest::Client::new();
    let params = [
        ("client_id", client_id),
        ("client_secret", client_secret),
        ("device_code", device_code),
        ("grant_type", "urn:ietf:params:oauth:grant-type:device_code"),
    ];

    let res = client
        .post("https://accounts.google.com/o/oauth2/token")
        .form(&params)
        .send()
        .await?
        .json::<TokenResponse>()
        .await?;

    Ok(res)
}

async fn upload_file(
    access_token: &str,
    file_path: &str,
) -> Result<String, Box<dyn std::error::Error>> {
    let client = reqwest::Client::new();

    // Leer el archivo y cargarlo como bytes
    let mut file = File::open(file_path).map_err(|e| Box::new(e) as Box<dyn std::error::Error>)?;
    let mut file_bytes = Vec::new();
    file.read_to_end(&mut file_bytes)
        .map_err(|e| Box::new(e) as Box<dyn std::error::Error>)?;

    let file_part = multipart::Part::bytes(file_bytes).mime_str("application/zip")?;
    let metadata = json!({
        "name": file_path,
    });
    let metadata_part =
        multipart::Part::text(metadata.to_string()).mime_str("application/json; charset=UTF-8")?;

    let form = multipart::Form::new()
        .part("metadata", metadata_part)
        .part("file", file_part);

    let res = client
        .post("https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart")
        .bearer_auth(access_token)
        .multipart(form)
        .send()
        .await?
        .text()
        .await?;

    Ok(res)
}
