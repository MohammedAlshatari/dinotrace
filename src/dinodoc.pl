#!/usr/local/bin/perl
# $Id$
#*****************************************************************************
# dinodoc.pl --- convert documentation text file to header file
# 
# This file is part of Dinotrace.  
# 
# Author: Wilson Snyder <wsnyder@world.std.com> or <wsnyder@ultranet.com>
# 
# Code available from: http://www.ultranet.com/~wsnyder/dinotrace
# 
#*****************************************************************************
# 
# This file is covered by the GNU public licence.
# 
# Dinotrace is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public Licens as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# Dinotrace is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Dinotrace; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
# 
#****************************************************************************/

print "/* Created automatically by dinodoc.pl from dinotrace.txt */\n";

print "\nchar dinodoc[] = \"\\\n";

# Every so often, end the string, as some compilers barf on long lines 
my $line = 0;

while(<STDIN>) {
    my $buf = $_;
    chop $buf;
    $buf =~ s/\\/{backslash}/g;
    $buf =~ s/{backslash}/\\\\/g;
    $buf =~ s/\"/\\\"/g;
    print "$buf\\n\\\n";
    
    $line++;
    if ($line > 30) {
	print "\"\n\"";
        $line = 0;
    }
}

print "\";\n\n";
