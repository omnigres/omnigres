import unittest
from check_extension_artifacts import check_extension_artifacts, OMNI_EXTENSION_PREFIX


class TestExtensionArtifacts(unittest.TestCase):
    def test_incorrect_version(self):
        """
        omnigres extension with incorrect version
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_sql=1.2
"""
            )

        self.assertIn(
            "extension 'omni_sql' version '1.2' is neither semver compliant nor 'unreleased'",
            str(e.exception),
        )

    def test_incorrect_dependency_version(self):
        """
        omnigres extension dependency with incorrect version
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_httpc=unreleased#omni_http=1.0
omni_http=1.0"""
            )

        self.assertIn(
            "'omni_http' dependency version '1.0' is neither semver nor 'unreleased'",
            str(e.exception),
        )

    def test_duplicate_extension(self):
        """
        duplicate extension entries
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_http=1.0.0
omni_httpc=unreleased#omni_http=1.0.0
omni_http=1.0.0
"""
            )

        self.assertIn(
            "extension 'omni_http' is at the beginning of more than one line",
            str(e.exception),
        )

    def test_different_extension_version(self):
        """
        different versions of same extension is not allowed
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_http=1.0.0
omni_httpc=unreleased#omni_http=1.3.0,omni_types=unreleased"""
            )

        self.assertIn(
            "version of 'omni_http' extension should be same throughout the file",
            str(e.exception),
        )

    def test_duplicate_dependencies(self):
        """
        duplicate dependencies of an extension
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_http=1.0.0
omni_httpc=unreleased#omni_http=1.0.0,omni_types=unreleased,omni_http=1.0.0"""
            )

        self.assertIn(
            "omni_http found more than once in omni_httpc dependencies",
            str(e.exception),
        )

    def test_cyclical_dependencies(self):
        """
        extension and it's dependencies form a cycle
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_httpd=1.2.1#omni_httpc=1.0.1
omni_httpc=1.0.1#omni_web=1.3.0
omni_web=1.3.0#omni_httpd=1.2.1"""
            )

        self.assertIn(
            "Extension dependency graph has a cycle! No topological ordering exists.",
            str(e.exception),
        )

    def test_missing_omnigres_extension(self):
        """
        every omnigres extension have it's own entry
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_httpd=1.2.1#omni_httpc=1.0.1
"""
            )

        self.assertIn(
            "'omni_httpc' extension doesn't have it's own entry in artifacts file",
            str(e.exception),
        )

    def test_non_omnigres_extension(self):
        """
        non omnigres extensions can only be in dependencies
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """pgcrypto=1.2.1#omni_os=1.1.0
omni_os=1.1.0"""
            )
        self.assertIn(
            f"non omnigres extension (doesn't have '{OMNI_EXTENSION_PREFIX}' prefix) 'pgcrypto' can only be in dependencies",
            str(e.exception),
        )

    def test_non_omnigres_dependency_extension(self):
        """
        non omnigres extensions can only be in dependencies with version '*'
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_os=1.1.0#pgcrypto=1.1.1
"""
            )
        self.assertIn(
            f"non omnigres dependency (doesn't have '{OMNI_EXTENSION_PREFIX}' prefix) "
            f"'pgcrypto' version should only be '*'",
            str(e.exception),
        )

    def test_released_extension_with_unreleased_dependency(self):
        """
        relased extension can't have unreleased dependencies
        """
        with self.assertRaises(Exception) as e:
            check_extension_artifacts(
                """omni_httpd=1.2.1#omni_var=unreleased
    omni_var=unreleased"""
            )
        self.assertIn(
            "'omni_httpd' extension with release version '1.2.1' "
            "can't have 'unreleased' dependency 'omni_var'",
            str(e.exception),
        )

    def test_unrelased_extension(self):
        """
        unrelased extension with both versioned and unreleased dependencies
        """
        extensions_to_release = check_extension_artifacts(
            """omni_httpd=1.2.1
omni_web=unreleased#omni_httpd=1.2.1,omni_var=unreleased
omni_httpc=1.2.3#omni_http=1.4.8
omni_http=1.4.8
omni_var=unreleased"""
        )
        self.assertEqual(
            extensions_to_release,
            ["omni_httpc=1.2.3", "omni_http=1.4.8", "omni_httpd=1.2.1"],
            "extensions to release doesn't match",
        )


if __name__ == "__main__":
    unittest.main()
