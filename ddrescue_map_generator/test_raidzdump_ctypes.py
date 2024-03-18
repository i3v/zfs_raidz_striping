from unittest import TestCase

from ddrescue_map_generator.raidzdump_ctypes import get_file_location


class Test_raidzdump(TestCase):

    def test_vdev_raidz_map_get(self):
        #   # ./raidzdump 14c00 20000 5
        #   cols = 5, firstdatacol = 1
        #   1:4200:8000
        #   2:4200:8000
        #   3:4200:8000
        #   4:4200:8000
        #   0:4400:8000

        loc = get_file_location(
            size = int('20000', 16),
            offset= int('14c00', 16),
            unit_shift=9,
            dcols=5,
            nparity=1
        )

        self.assertEqual(loc.num_cols, 5)
        self.assertEqual(loc.first_data_col, 1)
        self.assertEqual(len(loc.block_locations), 5)

        self.assertEqual([b.dev_idx for b in loc.block_locations], [1,2,3,4,0])
        self.assertEqual([b.offset for b in loc.block_locations],
                         [int('4200',16), int('4200', 16), int('4200', 16), int('4200',16), int('4400',16)])

        self.assertEqual([b.rc_size for b in loc.block_locations],
                         [int('8000', 16)] * 5)



