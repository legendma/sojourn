#!/usr/local/bin/python
import argparse
import os
import shutil
import glob
from distutils.dir_util import copy_tree
from winreg import *

DEFAULT_KEY = 'SOFTWARE\\Umbrellas Required\\Sojourn\\Main\\'
DEFAULT_APP_NAME = r"\Sojourn.exe"

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("install_folder", help="the folder to install the game into")
    parser.add_argument("app_folder", help = "the folder where the game executable is located")
    parser.add_argument("assets_folder", help = "the folder containing the game resources")
    args = parser.parse_args()

    print( "Installing to: ", args.install_folder )
    print( "app folder: ", args.app_folder )
    print( "assets folder: ", args.assets_folder )
    install_app( args.install_folder, args.app_folder, args.assets_folder)
    write_install_loc_to_registry(args.install_folder)


def install_app(install_folder, app_folder, assets_folder, app_name = DEFAULT_APP_NAME):
    # copy .exe and game assets folder to the install folder

    app_with_path = app_folder + app_name
    print( "App and path: ", app_with_path )
    if not os.path.exists(install_folder):
        os.mkdir(install_folder)
    shutil.copy2(app_with_path, install_folder)
    os.chdir(install_folder)
    assets_dest = install_folder + r"\assets"
    if not os.path.exists(assets_dest):
        os.mkdir(assets_dest)
  
    copy_tree(assets_folder, assets_dest)
    shaders_dest = assets_dest + r"\shaders"
    shaders_folder = app_folder + r"\*.cso"
    for file in glob.glob( shaders_folder ):
        print( file )
        shutil.copy( file, shaders_dest )

def write_install_loc_to_registry(install_folder, key = DEFAULT_KEY):
    # write registry key
    try:
        key_handle = CreateKey( HKEY_LOCAL_MACHINE, key )
    except WindowsError:
        print( "Failed to create key" )
        exit( 1 )

    SetValueEx( key_handle, "Installation Directory", 0, REG_SZ, install_folder )
    CloseKey( key_handle )

    
if __name__ == "__main__":
     main()
   