#!/usr/bin/env python

def options(opt):
    pass

def configure(conf):
    pass

def build(bld):
    src =  bld.path.ant_glob( 'libpsk/src/codec/*.cpp' )
    src += bld.path.ant_glob( 'libpsk/src/ham/*.cpp' )
    src += bld.path.ant_glob( 'libpsk/src/psk/*.cpp' )
    src += bld.path.ant_glob( 'libpsk/src/port/isix/*.cpp' )
    inc = ['libpsk/include' ]
    bld.stlib( source=src,
            includes = inc, export_includes=inc,
            target='psk', use=['foundation', 'stm32' ]
    )
