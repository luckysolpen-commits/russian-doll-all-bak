#!/usr/bin/env python3
import os
import glob
import sys

BOOK_DIRS = [
    "SP.all-writeez28-b2/0.Sol Pen]bk0-K-ARK-2.0/#.book_SP_0.full+aud-c2/",
    "SP.all-writeez28-b2/1.ii.geminii]2part]a0/book_part-i.gemini/",
    "SP.all-writeez28-b2/1.ii.geminii]2part]a0/book_part-ii.gemini]2smol.cho/",
    "SP.all-writeez28-b2/2.RedRock.AndROLL]ING]mid]a1/book_RR]BIG/",
    "SP.all-writeez28-b2/3.DEATH-KRYO!]big]b2/book_template.scp/",
    "SP.all-writeez28-b2/4.moonshine]big]b1/book_template.scp/",
    "SP.all-writeez28-b2/5.SUPERWAR.ikrs]FULwc]bk0/book_template.scp/",
    "SP.all-writeez28-b2/6.hellride-bk0]BIG]b1/book_template.scp/",
    "SP.all-writeez28-b2/7.AVANTA.C]mid]c6/book_.avanta.c/",
    "SP.all-writeez28-b2/8.Digi_Sol.Suicider]BIG]b1/8.DSS.book_]full-gemini]a0/",
    "SP.all-writeez28-b2/9.METAL SUN[METAL]wcsm+wtf]a1/ms_book]a0/",
    "SP.all-writeez28-b2/10.xo.files]wc.smol]a0/book_template.scp/",
    "SP.all-writeez28-b2/11.FOOTRACE.FU]HUGE/book_template.scp/",
    "SP.all-writeez28-b2/12.centroid.u.ϕ]2xvsp]big]b1/cent-book.bigword]a0/",
    "SP.all-writeez28-b2/333.chatoe.bnb.13]a0]FULL/",
    "SP.all-writeez28-b2/100.%.NGL.MPYRE]bk0/14.book]nglr.mpyre]FULL]b1/",
    "SP.all-writeez28-b2/111.tear-it]accidental.witch]y3/book_template.scp/"
]

def cleanup():
    count = 0
    for bdir in BOOK_DIRS:
        if not os.path.isdir(bdir):
            print(f"Skipping (not found): {bdir}")
            continue
        
        print(f"Cleaning: {bdir}")
        # Recursive delete of .mp3
        for root, dirs, files in os.walk(bdir):
            for f in files:
                if f.lower().endswith(".mp3"):
                    fpath = os.path.join(root, f)
                    try:
                        os.unlink(fpath)
                        count += 1
                    except Exception as e:
                        print(f"  Failed to delete {fpath}: {e}")
    
    print(f"\nTotal MP3s deleted: {count}")

if __name__ == "__main__":
    cleanup()
