use polling::{Event, Poller};
use std::io::{self, Write};
use std::net::{TcpListener, TcpStream};
use std::time::Duration;

#[test]
fn basic_io() {
    let poller = Poller::new().unwrap();
    let (read, mut write) = tcp_pair().unwrap();
    poller.add(&read, Event::readable(1)).unwrap();

    // Nothing should be available at first.
    let mut events = vec![];
    assert_eq!(
        poller
            .wait(&mut events, Some(Duration::from_secs(0)))
            .unwrap(),
        0
    );
    assert!(events.is_empty());

    // After a write, the event should be available now.
    write.write_all(&[1]).unwrap();
    assert_eq!(
        poller
            .wait(&mut events, Some(Duration::from_secs(1)))
            .unwrap(),
        1
    );
    assert_eq!(&*events, &[Event::readable(1)]);
}

fn tcp_pair() -> io::Result<(TcpStream, TcpStream)> {
    let listener = TcpListener::bind("127.0.0.1:0")?;
    let a = TcpStream::connect(listener.local_addr()?)?;
    let (b, _) = listener.accept()?;
    Ok((a, b))
}
