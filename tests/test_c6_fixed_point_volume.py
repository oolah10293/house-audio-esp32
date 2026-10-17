#!/usr/bin/env python3
"""Pure arithmetic tests for the C6 Q16.16 soft-volume patch (no ESPHome build)."""
import pathlib
import unittest

SOURCE = (pathlib.Path(__file__).resolve().parents[1] /
          "components/dsp_processor/dsp_processor.c")

def q16_gain(volume_percent: int) -> int:
    assert 0 <= volume_percent <= 100
    return int(volume_percent / 100 * 65536 + 0.5)

def scale_sample(sample: int, gain: int) -> int:
    assert -32768 <= sample <= 32767
    assert 0 <= gain <= 65536
    return int((sample * gain) / 65536)  # C signed division truncates toward zero.

def pack_stereo(left: int, right: int) -> int:
    return ((right & 0xffff) << 16) | (left & 0xffff)

def signed16(word: int) -> int:
    return word - 65536 if word & 0x8000 else word

class TestFixedPointVolume(unittest.TestCase):
    def test_gain_endpoints(self):
        self.assertEqual(q16_gain(0), 0)
        self.assertEqual(q16_gain(100), 65536)

    def test_signed_range_and_reference(self):
        samples = [-32768, -32767, -30000, -16385, -16384, -9999,
                   -1, 0, 1, 9999, 16384, 30000, 32767]
        for volume in range(101):
            gain = q16_gain(volume)
            for sample in samples:
                with self.subTest(volume=volume, sample=sample):
                    scaled = scale_sample(sample, gain)
                    original = int(sample * volume / 100)
                    self.assertLessEqual(abs(scaled - original), 1)
                    self.assertGreaterEqual(scaled, -32768)
                    self.assertLessEqual(scaled, 32767)
                    if volume == 100:
                        self.assertEqual(scaled, sample)

    def test_stereo_packing(self):
        for left, right in [(32767, -32768), (-1, 1),
                            (-22222, 12345), (0, 0)]:
            packed = pack_stereo(scale_sample(left, q16_gain(25)),
                                 scale_sample(right, q16_gain(25)))
            self.assertEqual(signed16(packed & 0xffff),
                             scale_sample(left, q16_gain(25)))
            self.assertEqual(signed16((packed >> 16) & 0xffff),
                             scale_sample(right, q16_gain(25)))

    def test_patched_source_has_atomic_gain(self):
        source = SOURCE.read_text(encoding="utf-8")
        self.assertIn("dynamic_vol_q16", source)
        self.assertIn("atomic_load_explicit", source)
        self.assertIn("atomic_store_explicit", source)
        self.assertIn("const int32_t left", source)
        self.assertIn("const int32_t right", source)

if __name__ == "__main__":
    unittest.main()
