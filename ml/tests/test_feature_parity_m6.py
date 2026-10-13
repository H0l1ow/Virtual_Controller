import sys
import unittest
from pathlib import Path
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from vcml.features import FeatureExtractor, geometry
from vcml.rules import scores


class M6ParityFixture(unittest.TestCase):
    def test_python_matches_native_frozen_fixture(self):
        points = [[.32 + (i % 4)*.033 + (i // 4)*.009,
                   .27 + (i % 5)*.049 + (i // 5)*.006,
                   .012*(i % 3)] for i in range(21)]
        hand = {'tracked': True, 'side': 'left', 'handedness_confidence': .95,
                'landmarks': points}
        f = FeatureExtractor().update(hand, 640, 480, 1_000_000)
        expected = {3:-.267575, 4:.297981, 5:.097300, 9:-.802726,
                    126:.446073, 127:.754623, 128:.754031, 129:.734067,
                    130:.288395, 131:.551620, 132:.95, 133:1.0}
        for index, value in expected.items():
            self.assertAlmostEqual(float(f[index]), value, delta=3e-5)
        g = geometry(hand, 640, 480)
        self.assertTrue(g.valid)
        self.assertAlmostEqual(float(scores(g)[1]), 1.0, delta=1e-6)

if __name__ == '__main__':
    unittest.main()
