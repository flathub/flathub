use anyhow::{bail, Context, Result};
use serde::{de::DeserializeOwned, Serialize};
use sha2::{Digest, Sha256};
use std::{
    env,
    fs,
    path::{Path, PathBuf},
};

const SERVICE_NAME: &str = "org.openkara.remote-library";

pub fn store_json<T: Serialize>(app_data_dir: &Path, library_id: &str, value: &T) -> Result<()> {
    let payload = serde_json::to_string(value).context("failed to serialize credential payload")?;
    store_string(app_data_dir, library_id, &payload)
}

pub fn load_json<T: DeserializeOwned>(app_data_dir: &Path, library_id: &str) -> Result<Option<T>> {
    let Some(payload) = load_string(app_data_dir, library_id)? else {
        return Ok(None);
    };
    let value = serde_json::from_str(&payload).context("failed to parse credential payload")?;
    Ok(Some(value))
}

pub fn delete(_app_data_dir: &Path, library_id: &str) -> Result<()> {
    if let Some(dir) = test_store_dir() {
        let path = test_store_path(&dir, library_id);
        if path.exists() {
            fs::remove_file(&path)
                .with_context(|| format!("failed to remove {}", path.display()))?;
        }
        return Ok(());
    }

    platform::delete(target_name(library_id))
}

fn store_string(app_data_dir: &Path, library_id: &str, payload: &str) -> Result<()> {
    if let Some(dir) = test_store_dir() {
        fs::create_dir_all(&dir)
            .with_context(|| format!("failed to create {}", dir.display()))?;
        let path = test_store_path(&dir, library_id);
        fs::write(&path, payload)
            .with_context(|| format!("failed to write {}", path.display()))?;
        let _ = app_data_dir;
        return Ok(());
    }

    platform::store(target_name(library_id), payload)
}

fn load_string(app_data_dir: &Path, library_id: &str) -> Result<Option<String>> {
    if let Some(dir) = test_store_dir() {
        let path = test_store_path(&dir, library_id);
        if !path.exists() {
            return Ok(None);
        }
        let payload = fs::read_to_string(&path)
            .with_context(|| format!("failed to read {}", path.display()))?;
        let _ = app_data_dir;
        return Ok(Some(payload));
    }

    platform::load(target_name(library_id))
}

fn target_name(library_id: &str) -> String {
    format!("{SERVICE_NAME}:{library_id}")
}

fn test_store_dir() -> Option<PathBuf> {
    env::var_os("OPENKARA_TEST_CREDENTIAL_STORE_DIR").map(PathBuf::from)
}

fn test_store_path(directory: &Path, library_id: &str) -> PathBuf {
    let digest = Sha256::digest(library_id.as_bytes());
    directory.join(format!("{:x}.json", digest))
}

#[cfg(target_os = "macos")]
mod platform {
    use super::*;
    use std::process::Command;

    pub fn store(target: String, payload: &str) -> Result<()> {
        let output = Command::new("security")
            .args([
                "add-generic-password",
                "-U",
                "-s",
                SERVICE_NAME,
                "-a",
                &target,
                "-w",
                payload,
            ])
            .output()
            .context("failed to launch macOS security CLI")?;
        if output.status.success() {
            return Ok(());
        }

        bail!(
            "OpenKara could not store remote credentials in the macOS Keychain: {}",
            String::from_utf8_lossy(&output.stderr).trim()
        )
    }

    pub fn load(target: String) -> Result<Option<String>> {
        let output = Command::new("security")
            .args([
                "find-generic-password",
                "-s",
                SERVICE_NAME,
                "-a",
                &target,
                "-w",
            ])
            .output()
            .context("failed to launch macOS security CLI")?;
        if output.status.success() {
            return Ok(Some(String::from_utf8(output.stdout)?.trim_end().to_owned()));
        }

        let stderr = String::from_utf8_lossy(&output.stderr);
        if stderr.contains("could not be found") || output.status.code() == Some(44) {
            return Ok(None);
        }

        bail!(
            "OpenKara could not read remote credentials from the macOS Keychain: {}",
            stderr.trim()
        )
    }

    pub fn delete(target: String) -> Result<()> {
        let output = Command::new("security")
            .args([
                "delete-generic-password",
                "-s",
                SERVICE_NAME,
                "-a",
                &target,
            ])
            .output()
            .context("failed to launch macOS security CLI")?;
        if output.status.success() {
            return Ok(());
        }

        let stderr = String::from_utf8_lossy(&output.stderr);
        if stderr.contains("could not be found") || output.status.code() == Some(44) {
            return Ok(());
        }

        bail!(
            "OpenKara could not remove remote credentials from the macOS Keychain: {}",
            stderr.trim()
        )
    }
}

#[cfg(target_os = "linux")]
mod platform {
    use super::*;
    use std::process::{Command, Stdio};

    const ATTR_SCOPE: &str = "openkara_scope";
    const ATTR_LIBRARY_ID: &str = "library_id";

    pub fn store(target: String, payload: &str) -> Result<()> {
        let mut child = Command::new("secret-tool")
            .args([
                "store",
                "--label=OpenKara remote library credentials",
                ATTR_SCOPE,
                SERVICE_NAME,
                ATTR_LIBRARY_ID,
                &target,
            ])
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()
            .context(linux_unavailable_message())?;
        if let Some(mut stdin) = child.stdin.take() {
            use std::io::Write;
            stdin
                .write_all(payload.as_bytes())
                .context("failed to write to secret-tool stdin")?;
        }
        let output = child
            .wait_with_output()
            .context("failed to wait for secret-tool")?;
        if output.status.success() {
            return Ok(());
        }

        bail!(
            "OpenKara could not store remote credentials in the Linux system keyring: {}. {}",
            String::from_utf8_lossy(&output.stderr).trim(),
            linux_unavailable_help()
        )
    }

    pub fn load(target: String) -> Result<Option<String>> {
        let output = Command::new("secret-tool")
            .args([
                "lookup",
                ATTR_SCOPE,
                SERVICE_NAME,
                ATTR_LIBRARY_ID,
                &target,
            ])
            .output()
            .context(linux_unavailable_message())?;
        if output.status.success() {
            let value = String::from_utf8(output.stdout)?.trim_end().to_owned();
            if value.is_empty() {
                return Ok(None);
            }
            return Ok(Some(value));
        }

        let stderr = String::from_utf8_lossy(&output.stderr);
        if stderr.trim().is_empty() {
            return Ok(None);
        }

        bail!(
            "OpenKara could not read remote credentials from the Linux system keyring: {}. {}",
            stderr.trim(),
            linux_unavailable_help()
        )
    }

    pub fn delete(target: String) -> Result<()> {
        let output = Command::new("secret-tool")
            .args([
                "clear",
                ATTR_SCOPE,
                SERVICE_NAME,
                ATTR_LIBRARY_ID,
                &target,
            ])
            .output()
            .context(linux_unavailable_message())?;
        if output.status.success() || String::from_utf8_lossy(&output.stderr).trim().is_empty() {
            return Ok(());
        }

        bail!(
            "OpenKara could not remove remote credentials from the Linux system keyring: {}. {}",
            String::from_utf8_lossy(&output.stderr).trim(),
            linux_unavailable_help()
        )
    }

    fn linux_unavailable_message() -> &'static str {
        "OpenKara could not access secret-tool. A Secret Service provider such as GNOME Keyring or KWallet must be installed and unlocked."
    }

    fn linux_unavailable_help() -> &'static str {
        "Install or unlock a desktop keyring provider, then reconnect the remote library."
    }
}

#[cfg(target_os = "windows")]
mod platform {
    use super::*;
    use std::{ffi::c_void, ptr};

    type Dword = u32;
    type Bool = i32;
    type Lpbyte = *mut u8;
    type Lpcwstr = *const u16;
    type Lpwstr = *mut u16;

    const CRED_TYPE_GENERIC: Dword = 1;
    const CRED_PERSIST_LOCAL_MACHINE: Dword = 2;
    const ERROR_NOT_FOUND: Dword = 1168;

    #[repr(C)]
    struct CredentialAttributeW {
        keyword: Lpwstr,
        flags: Dword,
        value_size: Dword,
        value: Lpbyte,
    }

    #[repr(C)]
    struct CredentialW {
        flags: Dword,
        type_: Dword,
        target_name: Lpwstr,
        comment: Lpwstr,
        last_written: [u32; 2],
        credential_blob_size: Dword,
        credential_blob: Lpbyte,
        persist: Dword,
        attribute_count: Dword,
        attributes: *mut CredentialAttributeW,
        target_alias: Lpwstr,
        user_name: Lpwstr,
    }

    #[link(name = "Advapi32")]
    extern "system" {
        fn CredWriteW(credential: *const CredentialW, flags: Dword) -> Bool;
        fn CredReadW(
            target_name: Lpcwstr,
            type_: Dword,
            flags: Dword,
            credential: *mut *mut CredentialW,
        ) -> Bool;
        fn CredDeleteW(target_name: Lpcwstr, type_: Dword, flags: Dword) -> Bool;
        fn CredFree(buffer: *mut c_void);
    }

    #[link(name = "Kernel32")]
    extern "system" {
        fn GetLastError() -> Dword;
    }

    pub fn store(target: String, payload: &str) -> Result<()> {
        let mut target_utf16 = to_utf16(&target);
        let mut username_utf16 = to_utf16("OpenKara");
        let mut blob = payload.as_bytes().to_vec();
        let credential = CredentialW {
            flags: 0,
            type_: CRED_TYPE_GENERIC,
            target_name: target_utf16.as_mut_ptr(),
            comment: ptr::null_mut(),
            last_written: [0, 0],
            credential_blob_size: blob.len() as Dword,
            credential_blob: blob.as_mut_ptr(),
            persist: CRED_PERSIST_LOCAL_MACHINE,
            attribute_count: 0,
            attributes: ptr::null_mut(),
            target_alias: ptr::null_mut(),
            user_name: username_utf16.as_mut_ptr(),
        };
        let ok = unsafe { CredWriteW(&credential, 0) };
        if ok != 0 {
            return Ok(());
        }

        bail!(
            "OpenKara could not store remote credentials in Windows Credential Manager (error {}).",
            unsafe { GetLastError() }
        )
    }

    pub fn load(target: String) -> Result<Option<String>> {
        let mut credential_ptr: *mut CredentialW = ptr::null_mut();
        let target_utf16 = to_utf16(&target);
        let ok = unsafe { CredReadW(target_utf16.as_ptr(), CRED_TYPE_GENERIC, 0, &mut credential_ptr) };
        if ok == 0 {
            let error = unsafe { GetLastError() };
            if error == ERROR_NOT_FOUND {
                return Ok(None);
            }
            bail!(
                "OpenKara could not read remote credentials from Windows Credential Manager (error {}).",
                error
            );
        }

        let credential = unsafe { &*credential_ptr };
        let bytes = unsafe {
            std::slice::from_raw_parts(
                credential.credential_blob,
                credential.credential_blob_size as usize,
            )
        };
        let value = String::from_utf8(bytes.to_vec())
            .map_err(|error| anyhow::anyhow!("failed to decode Windows credential payload: {error}"));
        unsafe { CredFree(credential_ptr.cast()) };
        let value = value?;
        Ok(Some(value))
    }

    pub fn delete(target: String) -> Result<()> {
        let target_utf16 = to_utf16(&target);
        let ok = unsafe { CredDeleteW(target_utf16.as_ptr(), CRED_TYPE_GENERIC, 0) };
        if ok != 0 {
            return Ok(());
        }

        let error = unsafe { GetLastError() };
        if error == ERROR_NOT_FOUND {
            return Ok(());
        }
        bail!(
            "OpenKara could not remove remote credentials from Windows Credential Manager (error {}).",
            error
        )
    }

    fn to_utf16(value: &str) -> Vec<u16> {
        value.encode_utf16().chain(std::iter::once(0)).collect()
    }
}
