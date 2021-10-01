/* cert.c
 *
 * Copyright 2021 cf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#define KEYS         "keys"
extern char *root_app;

void generate_keys_for (const char *id) {
  char root[256];
  char root_keys[256];
  char path[256];
  const char *home = getenv ("HOME");

  snprintf (root, 256, "%s/.nem", home);
	if (access (root, F_OK) == -1) {
		mkdir (root, S_IRWXU | S_IRWXG | S_IRWXO);
	}

  int rr = access (root, F_OK);

  snprintf (root_keys, 256, "%s/%s", root, KEYS);
	if (access (root_keys, F_OK) == -1) {
		mkdir (root_keys, S_IRWXU | S_IRWXG | S_IRWXO);
	}


	char *s;
	if ((s = strchr (id, '\n')) || (s = strchr (id, '\r'))) *s = 0;

	snprintf (path, 256, "%s/%s", root_keys, id);

	mkdir (path, S_IRWXU | S_IRWXG | S_IRWXO);

	X509 *x509p;
	EVP_PKEY *pkeyp;
	RSA *rsa;
	X509_NAME *name;
	BIGNUM *bn;

	pkeyp = EVP_PKEY_new ();
	x509p = X509_new ();
	rsa = RSA_new ();
	bn = BN_new ();
	BN_set_word (bn, RSA_F4);
	BIO *bio_private, *bio_public;

	int ret = RSA_generate_key_ex (rsa, 2048, bn, NULL);

	if (!EVP_PKEY_assign_RSA (pkeyp, rsa)) {
		fprintf (stderr, "can't assign rsa\n");
		exit (-1);
	}

	X509_set_version (x509p, 2);
	ASN1_INTEGER_set(X509_get_serialNumber (x509p), 0);
	X509_gmtime_adj (X509_get_notBefore (x509p), 0);
	X509_gmtime_adj (X509_get_notAfter (x509p), (long) 60 * 60 * 24 * 365);
	X509_set_pubkey (x509p, pkeyp);

	name = X509_get_subject_name (x509p);

	X509_NAME_add_entry_by_txt (name, "C", MBSTRING_ASC, "RU", -1, -1, 0);
	X509_NAME_add_entry_by_txt (name, "CO", MBSTRING_ASC, "FORGOTTEN SOUL", -1, -1, 0);
	X509_NAME_add_entry_by_txt (name, "CN", MBSTRING_ASC, "test.union", -1, -1, 0);

	X509_set_issuer_name (x509p, name);

	if (!X509_sign (x509p, pkeyp, EVP_md5 ())) {
		fprintf (stderr, "can't sign x509.\n");
		exit (-1);
	}

	char pem[256];
	snprintf (pem, 256, "%s/%s", path, "key.pem");
	FILE *fp;
	fp = fopen (pem, "w");
	PEM_write_PrivateKey (fp,
			pkeyp,
			NULL,
			NULL,
			0,
			NULL,
			NULL
			);
	fclose (fp);

	snprintf (pem, 256, "%s/%s", path, "pub.pem");
	fp = fopen (pem, "w");
	PEM_write_PUBKEY (fp, pkeyp);
	fclose (fp);

	BN_free (bn);
	RSA_free (rsa);
	EVP_PKEY_free (pkeyp);

}
