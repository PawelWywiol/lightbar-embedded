import unittest

from remote_selection import apply_remote, resolve_remote

ON = "CONFIG_APP_REMOTE_ENABLED=y\n"
OFF = "# CONFIG_APP_REMOTE_ENABLED is not set\n"


class ResolveRemoteTest(unittest.TestCase):
    def test_env_enables_over_layer(self):
        self.assertTrue(resolve_remote("1", [OFF]))

    def test_env_disables_over_layer(self):
        self.assertFalse(resolve_remote("0", [ON]))

    def test_last_layer_wins_without_env(self):
        self.assertTrue(resolve_remote(None, [OFF, "CONFIG_X=1\n", ON]))
        self.assertFalse(resolve_remote(None, [ON, OFF]))

    def test_disabled_by_default(self):
        self.assertFalse(resolve_remote(None, ["CONFIG_X=1\n"]))


class ApplyRemoteTest(unittest.TestCase):
    def test_replaces_existing_value(self):
        self.assertEqual(apply_remote("A=1\n" + OFF + "B=2\n", True), "A=1\n" + ON + "B=2\n")
        self.assertEqual(apply_remote(ON, False), OFF)

    def test_appends_missing_value(self):
        self.assertEqual(apply_remote("A=1\n", True), "A=1\n" + ON)
        self.assertEqual(apply_remote("A=1", False), "A=1\n" + OFF)


if __name__ == "__main__":
    unittest.main()
