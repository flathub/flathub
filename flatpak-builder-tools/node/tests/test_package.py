from flatpak_node_generator.package import SemVer


def test_semver_parsing() -> None:
    assert SemVer.parse('1.7.2') == SemVer(major=1, minor=7, patch=2)

    assert SemVer.parse('1123.3213.8943') == SemVer(
        major=1123, minor=3213, patch=8943, prerelease=None
    )

    assert SemVer.parse('12.34.56-alpha01') == SemVer(
        major=12, minor=34, patch=56, prerelease=SemVer.Prerelease(('alpha01',))
    )

    assert SemVer.parse('12.34.56-a.2.b+build') == SemVer(
        major=12,
        minor=34,
        patch=56,
        prerelease=SemVer.Prerelease(('a', 2, 'b')),
    )


def test_semver_cmp() -> None:
    assert SemVer.parse('1.2.3') < SemVer.parse('1.3.2')
    assert SemVer.parse('1.3.2') > SemVer.parse('1.2.3')
    assert SemVer.parse('1.2.3') == SemVer.parse('1.2.3')

    assert SemVer.parse('1.0.0-alpha') < SemVer.parse('1.0.0-alpha.1')
    assert SemVer.parse('1.0.0-alpha.1') < SemVer.parse('1.0.0-alpha.2')
    assert SemVer.parse('1.0.0-alpha.1') < SemVer.parse('1.0.0-alpha.x')
    assert SemVer.parse('1.0.0-alpha.x') < SemVer.parse('1.0.0-beta')
    assert SemVer.parse('1.0.0-alpha.1') < SemVer.parse('1.0.0-alpha.1.1')
    assert SemVer.parse('1.0.0-alpha+build1') == SemVer.parse('1.0.0-alpha+build2')
