"""
Executing "python languages/main.py <folder>" merges every language .JSON in
that folder into one silifig name under the "<folder>.sfg" filename.
"""


import os
import sys
import json
from typing import BinaryIO


SILIFIG_VER: bytes = b'\x00';

if __name__ == "__main__":
    if (len(sys.argv) == 1):
        print("You need to provide a folder");
    else:
        folder: str = sys.argv[1];
        listOfFilenames: list[str] = os.listdir("languages/" + folder);

        silifig: BinaryIO = open("languages/" + folder + ".sfg", "wb");
        content: bytearray = b'';
        dataLen: int = 0;

        for filename in listOfFilenames:
            file: BinaryIO = open("languages/" + folder + "/" + filename, "r", encoding = "utf8");
            jsonData: dict = json.loads(file.read());

            arrayOfText: list[bytes] = jsonData["text"];
            categoryLen: int = 0;

            for text in arrayOfText:
                length: int = len(text.encode("utf-8")) + 1;
                categoryLen += length;

            content += categoryLen.to_bytes(4, "little");
            content += len(arrayOfText).to_bytes(4, "little");

            for text in arrayOfText:
                utf8Txt = text.encode("utf-8") + b'\x00';
                content += len(utf8Txt).to_bytes(4, "little");
                content += utf8Txt;

            dataLen += categoryLen;
            file.close();

        entries: bytes = len(listOfFilenames).to_bytes(2, "little");
        silifig.write(SILIFIG_VER + b'\x00' + entries + b'\x01\x00\x00\x00' + dataLen.to_bytes(8, "little") + content);
        silifig.close();
