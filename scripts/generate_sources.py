import requests
import json
import base64
import gzip
import pprint
import os
from pathlib import Path
import errno
import sys
import base64
import jwcrypto.jwt, jwcrypto.jwk
import cryptography.x509
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
from cryptography.hazmat.primitives.asymmetric import padding
import pprint
from hashlib import sha256
import datetime
import yaml

url_template = "http://jagex-akamai.aws.snxd.com/direct6/launcher-win/pieces/{}/{}.solidpiece"
dirname = os.getcwd()

def mkdir_p(path):
    try:
        parent_path = Path(path)
        os.makedirs(parent_path.parent.absolute(), exist_ok=True)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def get_abs_path(relative_path):
    return os.path.join(dirname, relative_path)

def download_gzip_deflate_and_validate(url, filename, digest):
    print ("Downloading file from: {}".format(url))
    response = requests.get(url)
    if response.status_code == 200:
        content = response.content

        with open(filename, 'wb') as f:
            f.write(content)     

        print("Decompressed data saved to:", filename)
    else:
        print(f"Failed to download file: {response.status_code}")

def fetch_metafile(url):
    response = requests.get(url)
    if response.status_code == 200:
        return response.content.decode()
    else:
        raise Exception(f"Failed to fetch metafile, status code: {response.status_code}")

def decode_base64(encoded_str):
    try:
        # Ensure the string is padded to a multiple of 4 characters
        padded_str = encoded_str + '=' * ((4 - len(encoded_str) % 4) % 4)
        
        # Decode the base64 string
        decoded_bytes = base64.b64decode(padded_str)
        
        # Return the decoded bytes
        return decoded_bytes
    except binascii.Error:
        print("An error occurred: Incorrect padding or corrupted data.")
        return None
def cleanup():
    for p in Path(".").glob("*.solidpiece"):
        p.unlink()
    for p in Path(".").glob("combined_file"):
        p.unlink()

def main():
    # TODO make this grab the latest release instead of hard-coding.
    metafile_url = "http://jagex-akamai.aws.snxd.com/direct6/launcher-win/metafile/d589817a9dbde1cb1c6f1cde1e81b5284db1c5d0617577e3c3b987406ca2b50b/metafile.json"
    # This is the fingerprint of the certificate that signed the JWT we are using from the jagex CDN so we can validate we are trusting the right certificate chain.
    JAGEX_PACKAGE_CERTIFICATE_SHA256_HASH = "848bae7e92dc58570db50cdfc933a78204c1b00f05d64f753a307ebbaed2404f"

    metafile_json = requests.get(metafile_url)
    node = {
        'type': "extra-data",
        'url': metafile_url,
        'sha256': sha256(metafile_json.content).hexdigest(),
        'filename': 'metafile.json',
        'only-arches': ['x86_64'],
        'size': len(metafile_json.content)
    }

    # Load and deserialize JWT
    jwt = metafile_json.content.strip()
    jwt = jwcrypto.jwt.JWT(jwt=jwt.decode("ascii"))

    # Write the JWT to the file
    with open('metafile-source.yaml', "w") as jwt_file:
        yaml.dump(node, jwt_file, explicit_start=False, default_flow_style=False, sort_keys=False)

    # Deserialize the leaf certificate and validate the fingerprint of the certificate
    trust_path = jwt.token.jose_header.get("x5c", [])
    leaf_cert_b64 = trust_path[0]
    leaf_cert_sha256_hash = sha256(leaf_cert_b64.encode('utf8')).hexdigest()

    print ("Validating fingerprint of the certificate that signed the JWT...")
    if leaf_cert_sha256_hash != JAGEX_PACKAGE_CERTIFICATE_SHA256_HASH:
        raise Exception("The certificate in the JWT header does not match the expected fingerprint.")

    leaf_cert = cryptography.x509.load_der_x509_certificate(
        base64.b64decode(leaf_cert_b64))

    # Derive public key from the package cert and convert to JWK
    public_key = leaf_cert.public_key()
    public_key = public_key.public_bytes(Encoding.PEM, PublicFormat.PKCS1)
    public_key = jwcrypto.jwk.JWK.from_pem(public_key)

    # Validate JWT and access claims
    jwt.validate(public_key)
    print('''The jwt has validated against the certificate. 
        Issuer: {}
        Subject: {}
        Expiration UTC: {}
        '''.format(leaf_cert.issuer, leaf_cert.subject, leaf_cert.not_valid_after))

    # Build certificate chain
    trust_path = jwt.token.jose_header.get("x5c", [])
    trust_path = [
        cryptography.x509.load_der_x509_certificate(base64.b64decode(cert))
        for cert in trust_path
    ]

    # Verify certificate chain
    for i in range(len(trust_path) - 1):
        issuer_certificate = trust_path[i + 1]
        subject_certificate = trust_path[i]
        issuer_public_key = issuer_certificate.public_key()
        issuer_public_key.verify(
            subject_certificate.signature,
            subject_certificate.tbs_certificate_bytes,
            padding.PKCS1v15(),
            subject_certificate.signature_hash_algorithm,
        )
        # Verify certificate expiration
        current_time = datetime.datetime.utcnow()
        if current_time < issuer_certificate.not_valid_before or current_time > issuer_certificate.not_valid_after:
            raise Exception("Issuer certificate has expired.")

    try:
        global digest_list
        global pad_array
        global file_list
        
        verified_claims_json = json.loads(jwt.claims)

        digest_list = verified_claims_json.get("pieces").get("digests")
        file_list = verified_claims_json.get("files")
        pad_array = verified_claims_json.get("pad")

        downloaded_file_pieces = []

        for digest in digest_list:
            print(digest)
            digest_string = base64.b64decode(digest).hex()
            download_gzip_deflate_and_validate(url_template.format(digest_string[0:2], digest_string), digest_string + ".solidpiece", digest_string)
            downloaded_file_pieces.append(digest_string)

        sources = []
        print(digest_list)
        for item in downloaded_file_pieces:
            with open("{}.solidpiece".format(item), 'rb') as temp_file:
                node = {
                    'type': "extra-data",
                    'url': url_template.format(item[0:2], item),
                    'sha256': sha256(temp_file.read()).hexdigest(),
                    'filename': '{}.solidpiece'.format(item),
                    'only-arches': ['x86_64'],
                    'size': os.path.getsize("{}.solidpiece".format(item))
                }
                sources.append(node)
        print(sources)
        with open('solidpiece-sources.yaml', 'w', encoding='utf-8') as f:
            yaml.dump(sources, f, explicit_start=False, default_flow_style=False, sort_keys=False)             
        
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
    cleanup()