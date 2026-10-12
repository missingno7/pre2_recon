PRE2.EXE extracted from your uploaded PREH2/PRE2.SQZ.

How to use in DOSBox:

1. Copy PRE2.EXE into the PREH2 directory next to files like LEVEL1.SQZ, SPRITES.SQZ, MENU.SQZ.
2. Start DOSBox and mount the parent folder, for example:

   mount c C:\Games\Prehistorik2
   c:
   cd PREH2
   PRE2.EXE

I also included extract_pre2_from_sqz.py, so you can reproduce the extraction:

   python extract_pre2_from_sqz.py PRE2.SQZ PRE2.EXE

Notes:
- The input PRE2.SQZ uses the CDRUN alternate LZW variant.
- Output size: 87452 bytes.
- Output SHA-256: 4f25a6a32e24b577b64b93e79a161b23d5aadbb46b780d8e1928957e5be10dbb
