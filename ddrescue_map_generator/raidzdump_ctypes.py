import string
from ctypes import Structure, c_int, c_char_p, POINTER, cast, pointer, byref, cdll, c_uint64, c_uint8
from pathlib import Path
from typing import NamedTuple, List


class RaidzColCtype(Structure):
    _fields_ = [
        ('rc_devidx', c_uint64),
        ('rc_offset', c_uint64),
        ('rc_size', c_uint64),
        ('rc_data', POINTER(c_char_p)),
        ('rc_gdata', POINTER(c_char_p)),
        ('rc_error', c_int),
        ('rc_tried', c_uint8),
        ('rc_skipped', c_uint8),
    ]

class RaidzMapCtype(Structure):
    _fields_ = [
        ('rm_cols', c_uint64),
        ('rm_scols', c_uint64),
        ('rm_bigcols', c_uint64),
        ('rm_asize', c_uint64),
        ('rm_missingdata', c_uint64),
        ('rm_missingparity', c_uint64),
        ('rm_firstdatacol', c_uint64),
        ('rm_nskip', c_uint64),
        ('rm_skipstart', c_uint64),
        ('rm_datacopy', POINTER(c_char_p)),
        ('rm_reports', c_uint64),
        ('rm_freed', c_uint8),
        ('rm_ecksuminjected', c_uint8),
        ('rm_col', RaidzColCtype),  # flex array
    ]

__self_folder = Path(__file__).parent
dll = cdll.LoadLibrary(str(__self_folder.parent.joinpath('raidzdump','bin','libraidzdump.so')))
dll.vdev_raidz_map_get.restype = POINTER(RaidzMapCtype)
dll.vdev_raidz_map_get.argtypes = [c_uint64, c_uint64, c_uint64, c_uint64, c_uint64]


class RaidzBlockLocation(NamedTuple):
    dev_idx: int
    offset: int
    rc_size: int

class RaidzFileLocation:
    def __init__(self, num_cols:int, first_data_col:int, block_locations:List[RaidzBlockLocation]):
        self.num_cols = num_cols
        self.first_data_col = first_data_col
        self.block_locations = block_locations


def get_file_location(size, offset, unit_shift, dcols, nparity):
    rmz: RaidzMapCtype = dll.vdev_raidz_map_get(size, offset, unit_shift, dcols, nparity)[0]
    bl = []
    col_ptr = pointer(rmz.rm_col)
    for i_col in range(rmz.rm_cols):
        col = col_ptr[i_col]
        bl.append(RaidzBlockLocation(dev_idx=col.rc_devidx, offset=col.rc_offset, rc_size=col.rc_size))
    rm = RaidzFileLocation(num_cols=rmz.rm_cols, first_data_col=rmz.rm_firstdatacol, block_locations=bl)
    return rm
