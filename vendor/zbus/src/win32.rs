use std::{
    ffi::{CStr, OsStr},
    io::{Error, ErrorKind},
    net::SocketAddr,
    os::windows::prelude::OsStrExt,
    ptr,
};

use winapi::{
    shared::{
        minwindef::{DWORD, FALSE},
        sddl::ConvertSidToStringSidA,
        tcpmib::{MIB_TCPTABLE2, MIB_TCP_STATE_ESTAB},
        winerror::{ERROR_INSUFFICIENT_BUFFER, NO_ERROR},
        ws2def::INADDR_LOOPBACK,
    },
    um::{
        handleapi::CloseHandle,
        iphlpapi::GetTcpTable2,
        memoryapi::{MapViewOfFile, OpenFileMappingW, FILE_MAP_READ},
        processthreadsapi::{GetCurrentProcess, OpenProcess, OpenProcessToken},
        securitybaseapi::{GetTokenInformation, IsValidSid},
        synchapi::{CreateMutexW, ReleaseMutex, WaitForSingleObject},
        winbase::{LocalFree, INFINITE, WAIT_ABANDONED, WAIT_OBJECT_0},
        winnt::{TokenUser, HANDLE, PROCESS_QUERY_LIMITED_INFORMATION, TOKEN_QUERY, TOKEN_USER},
    },
};

use crate::Address;
#[cfg(not(feature = "tokio"))]
use uds_windows::UnixStream;

// An owned Windows handle
pub struct OwnedHandle(HANDLE);

impl OwnedHandle {
    // SAFETY: since `handle` is just a pointer, it can be given to multiple `OwnedHandle`
    pub unsafe fn new(handle: HANDLE) -> Self {
        Self(handle)
    }

    #[inline]
    pub fn get(&self) -> HANDLE {
        self.0
    }
}

impl Drop for OwnedHandle {
    fn drop(&mut self) {
        unsafe { CloseHandle(self.0) };
    }
}

struct Mutex(OwnedHandle);

impl Mutex {
    pub fn new(name: &str) -> Result<Self, crate::Error> {
        let name_wide = OsStr::new(name)
            .encode_wide()
            .chain([0])
            .collect::<Vec<_>>();

        let handle = unsafe { CreateMutexW(ptr::null_mut(), FALSE, name_wide.as_ptr()) };

        // SAFETY: We have exclusive ownership over the mutex handle
        Ok(Self(unsafe { OwnedHandle::new(handle) }))
    }

    pub fn lock(&self) -> MutexGuard<'_> {
        match unsafe { WaitForSingleObject(self.0.get(), INFINITE) } {
            WAIT_ABANDONED | WAIT_OBJECT_0 => MutexGuard(self),
            err => panic!("WaitForSingleObject() failed: return code {}", err),
        }
    }
}

struct MutexGuard<'a>(&'a Mutex);

impl Drop for MutexGuard<'_> {
    fn drop(&mut self) {
        unsafe { ReleaseMutex(self.0 .0.get()) };
    }
}

// A process handle
pub struct ProcessHandle(OwnedHandle);

impl ProcessHandle {
    // Open the process associated with the process_id (if None, the current process)
    pub fn open(process_id: Option<DWORD>, desired_access: DWORD) -> Result<Self, Error> {
        let process = if let Some(process_id) = process_id {
            unsafe { OpenProcess(desired_access, false.into(), process_id) }
        } else {
            unsafe { GetCurrentProcess() }
        };

        if process.is_null() {
            Err(Error::last_os_error())
        } else {
            // SAFETY: We have exclusive ownership over the process handle
            Ok(Self(unsafe { OwnedHandle::new(process) }))
        }
    }
}

// A process token
//
// See MSDN documentation:
// https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocesstoken
//
// Get the process security identifier with the `sid()` function.
pub struct ProcessToken(OwnedHandle);

impl ProcessToken {
    // Open the access token associated with the process_id (if None, the current process)
    pub fn open(process_id: Option<DWORD>) -> Result<Self, Error> {
        let mut process_token: HANDLE = ptr::null_mut();
        let process = ProcessHandle::open(process_id, PROCESS_QUERY_LIMITED_INFORMATION)?;

        if unsafe { OpenProcessToken(process.0.get(), TOKEN_QUERY, &mut process_token) } == 0 {
            Err(Error::last_os_error())
        } else {
            // SAFETY: We have exclusive ownership over the process handle
            Ok(Self(unsafe { OwnedHandle::new(process_token) }))
        }
    }

    // Return the process SID (security identifier) as a string
    pub fn sid(&self) -> Result<String, Error> {
        let mut len = 256;
        let mut token_info;

        loop {
            token_info = vec![0u8; len as usize];

            let result = unsafe {
                GetTokenInformation(
                    self.0.get(),
                    TokenUser,
                    token_info.as_mut_ptr() as *mut _,
                    len,
                    &mut len,
                )
            };

            if result != 0 {
                break;
            }

            let last_error = Error::last_os_error();
            if last_error.raw_os_error() == Some(ERROR_INSUFFICIENT_BUFFER as i32) {
                continue;
            }

            return Err(last_error);
        }

        let sid = unsafe { (*(token_info.as_ptr() as *const TOKEN_USER)).User.Sid };

        if unsafe { IsValidSid(sid as *mut _) } == 0 {
            return Err(Error::new(ErrorKind::Other, "Invalid SID"));
        }

        let mut pstr: *mut i8 = ptr::null_mut();
        if unsafe { ConvertSidToStringSidA(sid as *mut _, &mut pstr as *mut _) } == 0 {
            return Err(Error::last_os_error());
        }

        let sid = unsafe { CStr::from_ptr(pstr) };
        let ret = sid
            .to_str()
            .map_err(|_| Error::new(ErrorKind::Other, "Invalid SID"))?;
        unsafe {
            LocalFree(pstr as *mut _);
        }

        Ok(ret.to_owned())
    }
}

// Get the process ID of the local socket address
// TODO: add ipv6 support
pub fn socket_addr_get_pid(addr: &SocketAddr) -> Result<DWORD, Error> {
    let mut len = 4096;
    let mut tcp_table = vec![];
    let res = loop {
        tcp_table.resize(len as usize, 0);
        let res =
            unsafe { GetTcpTable2(tcp_table.as_mut_ptr().cast::<MIB_TCPTABLE2>(), &mut len, 0) };
        if res != ERROR_INSUFFICIENT_BUFFER {
            break res;
        }
    };
    if res != NO_ERROR {
        return Err(Error::last_os_error());
    }

    let tcp_table = tcp_table.as_mut_ptr() as *const MIB_TCPTABLE2;
    let num_entries = unsafe { (*tcp_table).dwNumEntries };
    for i in 0..num_entries {
        let entry = unsafe { (*tcp_table).table.get_unchecked(i as usize) };
        let port = (entry.dwLocalPort & 0xFFFF) as u16;
        let port = u16::from_be(port);

        if entry.dwState == MIB_TCP_STATE_ESTAB
            && u32::from_be(entry.dwLocalAddr) == INADDR_LOOPBACK
            && u32::from_be(entry.dwRemoteAddr) == INADDR_LOOPBACK
            && port == addr.port()
        {
            return Ok(entry.dwOwningPid);
        }
    }

    Err(Error::new(ErrorKind::Other, "PID of TCP address not found"))
}

// Get the process ID of the connected peer
#[cfg(any(test, not(feature = "tokio")))]
pub fn tcp_stream_get_peer_pid(stream: &std::net::TcpStream) -> Result<DWORD, Error> {
    let peer_addr = stream.peer_addr()?;

    socket_addr_get_pid(&peer_addr)
}

#[cfg(any(test, not(feature = "tokio")))]
fn last_err() -> std::io::Error {
    use winapi::um::winsock2::WSAGetLastError;

    let err = unsafe { WSAGetLastError() };
    std::io::Error::from_raw_os_error(err)
}

// Get the process ID of the connected peer
#[cfg(not(feature = "tokio"))]
pub fn unix_stream_get_peer_pid(stream: &UnixStream) -> Result<DWORD, Error> {
    use std::os::windows::io::AsRawSocket;
    use winapi::{
        shared::ws2def::IOC_VENDOR,
        um::winsock2::{WSAIoctl, SOCKET_ERROR},
    };

    macro_rules! _WSAIOR {
        ($x:expr, $y:expr) => {
            winapi::shared::ws2def::IOC_OUT | $x | $y
        };
    }

    let socket = stream.as_raw_socket();
    const SIO_AF_UNIX_GETPEERPID: DWORD = _WSAIOR!(IOC_VENDOR, 256);
    let mut ret = 0 as DWORD;
    let mut bytes = 0;

    let r = unsafe {
        WSAIoctl(
            socket as _,
            SIO_AF_UNIX_GETPEERPID,
            0 as *mut _,
            0,
            &mut ret as *mut _ as *mut _,
            std::mem::size_of_val(&ret) as DWORD,
            &mut bytes,
            0 as *mut _,
            None,
        )
    };

    if r == SOCKET_ERROR {
        return Err(last_err());
    }

    Ok(ret)
}

fn read_shm(name: &str) -> Result<Vec<u8>, crate::Error> {
    let handle = {
        let wide_name = OsStr::new(name)
            .encode_wide()
            .chain([0])
            .collect::<Vec<_>>();

        let res = unsafe { OpenFileMappingW(FILE_MAP_READ, FALSE, wide_name.as_ptr()) };

        if !res.is_null() {
            // SAFETY: We have exclusive ownership over the file mapping handle
            unsafe { OwnedHandle::new(res) }
        } else {
            return Err(crate::Error::Address(
                "Unable to open shared memory".to_owned(),
            ));
        }
    };

    let addr = unsafe { MapViewOfFile(handle.get(), FILE_MAP_READ, 0, 0, 0) };

    if addr.is_null() {
        return Err(crate::Error::Address("MapViewOfFile() failed".to_owned()));
    }

    let data = unsafe { CStr::from_ptr(addr as *const _) };
    Ok(data.to_bytes().to_owned())
}

pub fn windows_autolaunch_bus_address() -> Result<Address, crate::Error> {
    let mutex = Mutex::new("DBusAutolaunchMutex")?;
    let _guard = mutex.lock();

    let addr = read_shm("DBusDaemonAddressInfo")?;
    let addr = String::from_utf8(addr)
        .map_err(|e| crate::Error::Address(format!("Unable to parse address as UTF-8: {}", e)))?;

    addr.parse()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn socket_pid_and_sid() {
        let listener = std::net::TcpListener::bind("127.0.0.1:0").unwrap();
        let addr = listener.local_addr().unwrap();
        let client = std::net::TcpStream::connect(addr).unwrap();
        let _server = listener.incoming().next().unwrap().unwrap();

        let pid = tcp_stream_get_peer_pid(&client).unwrap();
        let process_token = ProcessToken::open(if pid != 0 { Some(pid) } else { None }).unwrap();
        let sid = process_token.sid().unwrap();
        assert!(!sid.is_empty());
    }
}
