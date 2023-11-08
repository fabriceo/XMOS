/*
 * XCport.xc
 *
 *  Created on: 21 juin 2019
 *      Author: Fabriceo
 *
 */


#include "XCport.h"

// #include <stdio.h> // for printf only

/****************************/
// XC wrappers for calling interface as I couldnt find a c library to handle interface calls


void XCpinModeIF(client interface XCportTile_if ?tileIF, unsigned p, int mode){
    tileIF.pinModeBase(p, mode);
}
void XCdigitalWriteIF(client interface XCportTile_if ?tileIF, unsigned p, int val){
    tileIF.digitalWriteBase(p, val);
}

int  XCdigitalReadIF(client interface XCportTile_if ?tileIF, unsigned p){
    return tileIF.digitalReadBase(p);
}

/****************************/

// set the global variable with adress of interface ressource and tile currently used
void XCportInitTileIF(client XCPORT_TILE_IF(?tileIF), unsigned n){
    unsafe { portInitTileIF(
            (unsigned)tileIF, n,
            (get_local_tile_id() != get_tile_id(tile[0]) ) ); } // tile[0] comes from platform.h
}

// initialize a list of port with their mode, same as a loop of pinMode(x,y)
void XCportModeList(const unsigned int list[]){
    int i =0;
    int p, mode;
    if (!isnull(list))
    while ((p = (list[i++]))) { // will stop when 0 is met as an end of list
        mode = list[i++];
        pinMode(p, mode);
    }
}

// used to acces ports on the other tile. can be distributed on other cores
[[combinable]] void XCportTileTask(server XCPORT_TILE_IF(tileIF)[n], static const unsigned n) {
    while (1) {
        select {
        // using macro expansion here, because compiler do not accept distributable select functions ...
            case XCPORT_TILE_EVENTS(tileIF[int i]);
        }
    }
}
