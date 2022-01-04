#!/usr/bin/python3

import pysftp
import xml.etree.ElementTree as ET
import os
import shutil
import re
import argparse

"""
PATH = "/mnt/bete/Philipp/Roms"
USERNAME = "pi"
PASSWORD = "raspberry"
SERVER_ADDRESS = '192.168.0.2'
EXCHANGE_STR = "^~/RetroPie/roms("
"""


PATH = "/home/pi/RetroPie/roms"
USERNAME = "pi"
PASSWORD = "horbermatt"
SERVER_ADDRESS = '192.168.0.17'
EXCHANGE_STR = "^/home/philipp/RetroPie/roms/"


def rename_ressource_paths(elem, system, exchange_str=EXCHANGE_STR):
    cnt_paths = 0
    cnt_images = 0
    cnt_marquees = 0
    p = elem.find("path")
    if p is not None:
        if p.text is not None:
            p.text = re.sub(exchange_str + system, ".", p.text)
            cnt_paths += 1
    p = elem.find("image")
    if p is not None:
        if p.text is not None:
            p.text = re.sub(exchange_str + system, ".", p.text)
            cnt_images += 1
    p = elem.find("marquee")
    if p is not None:
        if p.text is not None:
            p.text = re.sub(exchange_str + system, ".", p.text)
            cnt_marquees += 1
    return cnt_paths, cnt_images, cnt_marquees


def process_gamelist(rp_system, fname=None):
    ex_str=None
    if fname is not None:
        shutil.copyfile(fname, "gamelist.xml")
        ex_str = "^/{}/".format(os.path.join(*fname.split(os.path.sep)[:-2]))
    gamelist = ET.parse("gamelist.xml")
    os.remove("gamelist.xml")
    root = gamelist.getroot()
    print("System: {}".format(rp_system))
    c_paths = 0
    c_images = 0
    c_marquees = 0
    c_games = 0
    games = root.findall("game")
    for g in games:
        c_games += 1
        updated_counter = rename_ressource_paths(g, rp_system, exchange_str=ex_str)
        c_paths += updated_counter[0]
        c_images += updated_counter[1]
        c_marquees += updated_counter[2]
    folders = root.findall("folder")
    for g in folders:
        c_games += 1
        updated_counter = rename_ressource_paths(g, rp_system, exchange_str=ex_str)
        c_paths += updated_counter[0]
        c_images += updated_counter[1]
        c_marquees += updated_counter[2]

    print("Total games: {}, Update {} paths, {} images and {} marquees".format(c_games, c_paths, c_images,
                                                                               c_marquees))
    gamelist.write("gamelist.xml")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert media path from absolute path to relative paths")
    parser.add_argument("-l", nargs="?", default=None, help="local path to gamelist.xml, doesn't connect ssh if given")

    args = parser.parse_args()
    if args.l is None:
        with pysftp.Connection(SERVER_ADDRESS, username=USERNAME, password=PASSWORD) as f:
            ld = f.listdir(PATH)
            for el in ld:
                if not el.endswith(".zip"):
                    romlist = f.listdir(PATH + "/" + el)
                    if "gamelist.xml" in romlist:
                        if os.path.exists(el):
                            shutil.rmtree(el)
                        os.mkdir(el)
                        currpath = PATH + "/" + el + "/gamelist.xml"
                        f.get(currpath)
                        shutil.copyfile("gamelist.xml", el + "/gamelist.xml")
                        process_gamelist(el)
                        resp = input("upload?")
                        if resp == "y":
                            f.remove(currpath)
                            f.put("gamelist.xml", currpath)
                pass
    else:
        el = args.l.split(os.path.sep)[-2]
        process_gamelist(el, args.l)
        shutil.copyfile("gamelist.xml", args.l)
