## CHECK THE [DOCUMENTATION](https://nichokas.github.io/g-up-docs/client-id-and-client-secret.html) FOR CREATING THE CLIENT_ID AND THE CLIENT_SECRET

# g-up

`g-up` is a command-line tool to quickly and easily upload files to Google Drive from the terminal. This tool is designed to be simple and efficient, allowing you to manage your files on Google Drive directly from the console.

## Installation

You can clone the repository and then install the necessary dependencies:

```bash
git clone https://github.com/Nichokas/g-up.git
cd g-up
cargo run build --release
```

## Usage

To upload a file to Google Drive, simply use the following command:

```bash
./target/release/g-up
```

### Examples

Upload a .zip:

```bash
$ ./g-up 
Go to https://www.google.com/device and enter the code: ***-***-****
Press Enter after authenticating...

Please enter the name of the .zip file to upload: 
test.zip
Access token received: *****
File uploaded successfully: {
  "kind": "drive#file",
  "id": "*****",
  "name": "test.zip",
  "mimeType": "application/zip"
}
```

## TODO

- [x] Added support for `.zip` files.
- [x] Create [documentation](https://nichokas.github.io/g-up-docs/client-id-and-client-secret.html)
- [ ] Add more filetypes compatibility
- [ ] Save the upload token

## Contributions

Contributions are welcome. If you have ideas to improve `g-up` or find any issues, feel free to open an issue or submit a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for more details.
