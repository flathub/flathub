from flatpak_node_generator.integrity import Integrity, IntegrityBuilder

TEST_STRING = 'this is a test string'
TEST_SHA1 = '9a375f77abb15794900c2689812204273d757c9b'
TEST_SHA256 = 'f6774519d1c7a3389ef327e9c04766b999db8cdfb85d1346c471ee86d65885bc'
TEST_SHA256_B64 = '9ndFGdHHozie8yfpwEdmuZnbjN+4XRNGxHHuhtZYhbw='


def test_generate() -> None:
    integrity = Integrity.generate(TEST_STRING)
    assert integrity.algorithm == 'sha256'
    assert integrity.digest == TEST_SHA256

    integrity = Integrity.generate(TEST_STRING, algorithm='sha1')
    assert integrity.algorithm == 'sha1'
    assert integrity.digest == TEST_SHA1


def test_builder() -> None:
    STEP_SIZE = 3

    builder = IntegrityBuilder()
    for i in range(0, len(TEST_STRING), STEP_SIZE):
        builder.update(TEST_STRING[i : i + STEP_SIZE])

    integrity = builder.build()
    assert integrity.algorithm == 'sha256'
    assert integrity.digest == TEST_SHA256


def test_parse() -> None:
    integrity = Integrity.parse(f'sha256-{TEST_SHA256_B64}')
    assert integrity.algorithm == 'sha256'
    assert integrity.digest == TEST_SHA256
