#!/usr/local/bin/tclsh
########################################################################
#
# This is a pure TCL implementation of the module command


########################################################################
# commands run from inside a module file
#

set ignoreDir(CVS) 1

proc module-info {what {more {}}} {
    upvar g_mode mode

    switch $what {
	"name" {
	    upvar currentModule currentModule
	    return $currentModule
	}
	"mode" {
	    if {$more != ""} {
		if {$mode == $more} {
		    return 1
		} else {
		    return 0
		}
	    } else {
		return $mode
	    }
	}
	default {
	    error "module-info $what not supported"
	}
    }
}

proc module-whatis {message} {
    upvar g_mode  mode
    upvar g_whatis whatis

    if {$mode == "display"} {
	puts stderr "module-whatis\t$message"
    } elseif {$mode == "whatis"} {
	set whatis $message
    }
}

proc module {command args} {
    upvar g_mode mode

    switch $command {
	load {
	    if {$mode == "load"} {
		cmdModuleLoad $args
	    } elseif {$mode == "unload"} {
		cmdModuleUnload $args
	    } elseif {$mode == "display"} {
		puts stderr "module load $args"
	    }
	}
	unload {
	    if {$mode == "load"} {
		cmdModuleUnload $args
	    } elseif {$mode == "unload"} {
		cmdModuleUnload $args
	    } elseif {$mode == "display"} {
		puts stderr "module unload $args"
	    }
	}
	default {
	    error "module $command not understood"
	}
    }
}

proc setenv {var val} {
    global g_stateEnvVars env 
    upvar g_mode mode

    if {$mode == "load"} {
	set env($var) $val
	set g_stateEnvVars($var) "new"
    } elseif {$mode == "unload"} {
	if [info exists env($var)] {
	    unset env($var)
	}
	set g_stateEnvVars($var) "del"
    } elseif {$mode == "display"} {
	puts stderr "setenv\t$var\t$val"
    }
}

proc unsetenv {var} {
    global g_stateEnvVars env 
    upvar g_mode mode
    
    if {$mode == "load"} {
	if [info exists env($var)] {
	    unset env($var)
	}
	set g_stateEnvVars($var) "del"
    } elseif {$mode == "display"} {
	puts stderr "unsetenv\t$var"
    }
}

########################################################################
# path fiddling

proc unload-path {var path} {
    global g_stateEnvVars env

    set sharevar "${var}_modshare"
    if [info exists env($sharevar)] {
	array set countarr [split $env($sharevar) ":"]
    }
    
    set doit 0

    foreach dir [split $path ":"] {
	if [info exists countarr($dir)] {
	    incr countarr($dir) -1
	    if {$countarr($dir) <= 0} {
		set doit 1
		unset countarr($dir)
	    }
	} else {
	    set doit 1
	}

	if {$doit} {
	    if [info exists env($var)] {
		set dirs [split $env($var) ":"]
		set newpath ""
		foreach elem $dirs {
		    if {$elem != $dir} {
			lappend newpath $elem
		    }
		}
		if {$newpath == ""} {
		    unset env($var)
		    set g_stateEnvVars($var) "del"
		} else {
		    set env($var) [join $newpath ":"]
		    set g_stateEnvVars($var) "new"
		}
	    }
	}
    }

    set env($sharevar) [join [array get countarr] ":"]
    set g_stateEnvVars($sharevar) "new"
}

proc add-path {var path pos} {
    global env g_stateEnvVars

    set sharevar "${var}_modshare"
    
    if [info exists env($sharevar)] {
	array set countarr [split $env($sharevar) ":"]
    }
    
    if [info exists env($var)] {
	if {$pos == "prepend"} {
	    set env($var) "$path:$env($var)"
	} elseif {$pos == "append"} {
	    set env($var) "$env($var):$path"
	} else {
	    error
	}
    } else {
	set env($var) "$path"
    }
    foreach dir [split $path ":"] {
	if [info exists countarr($dir)] {
	    incr countarr($dir)
	} else {
	    set countarr($dir) 1
	}
    }

    set env($sharevar) [join [array get countarr] ":"]
    set g_stateEnvVars($var) "new"
    set g_stateEnvVars($sharevar) "new"
}

proc prepend-path {var path} {
    upvar g_mode mode

    if {$mode == "load"} {
	add-path $var $path "prepend"
    } elseif {$mode == "unload"} {
	unload-path $var $path
    } elseif {$mode == "display"} {
	puts stderr "prepend-path\t$var\t$path"
    }
}


proc append-path {var path} {
    upvar g_mode mode

    if {$mode == "load"} {
	add-path $var $path "append"
    } elseif {$mode == "unload"} {
	unload-path $var $path
    } elseif {$mode == "display"} {
	puts stderr "append-path\t$var\t$path"
    }
}

proc set-alias {alias what} {
    global g_Aliases g_stateAliases 
    upvar g_mode mode

    if {$mode == "load"} {
	set g_Aliases($alias) $what
	set g_stateAliases($alias) "new"
    } elseif {$mode == "unload"} {
	set g_Aliases($alias) $what
	set g_stateAliases($alias) "del"
    } elseif {$mode == "display"} {
	puts stderr "alias\t$alias\t$what"
    }
}


proc conflict {args} {
    global g_loadedModules g_loadedModulesGeneric 
    upvar g_mode mode
    upvar currentModule currentModule

    if {$mode == "load"} {
	foreach conflict $args {
	    set mod [file dirname $conflict]
#puts stderr "mod=$mod"
	    if {$mod == "."} {
		if { [info exists g_loadedModulesGeneric($conflict) ] } {
		    set x $conflict/$g_loadedModulesGeneric($conflict)
		    if { $x != $currentModule } {
			error "Conflict with loaded module $x \nHINT: Do 'module unload $x' and then try again."
		    }
		}
	    } else {
		if { [info exists g_loadedModules($conflict)] &&
		     $conflict != $currentModule } {
		    error "Conflict ($currentModule) with loaded module: $conflict"
		}
	    }
	}
    } elseif {$mode == "display"} {
	puts stderr "conflict\t$args"
    }
}

proc x-resource {resource {value {}}} {
    global g_newXResources g_delXResources 
    upvar g_mode mode
    
    if {$mode == "load"} {
	set g_newXResources($resource) $value
    } elseif {$mode =="unload"} {
	set g_delXResources($resource) 1
    } elseif {$mode == "display"} {
	puts stderr "x-resource\t$resource\t$value"
    }
}

proc uname {what} {
    global unameCache
    if { ! [info exists unameCache($what)] } {
	switch $what {
	    sysname {
		catch { exec uname -s } result
	    }
	    machine {
		catch { exec uname -p } result
	    }
	    node {
		catch { exec uname -n } result
	    }
	    release {
		catch { exec uname -r } result
	    }
	    default {
		error "uname setting $what not implemented"
	    }
	}
	set unameCache($what) $result
    }
    
    return $unameCache($what)
}

########################################################################
# internal module procedures

proc getPathToModule {mod} {
    global env g_loadedModulesGeneric

    if [info exists env(MODULEPATH)] {
	foreach dir [split $env(MODULEPATH) ":"] {
	    set path "$dir/$mod"
	    if [file exists $path] {
		if [file readable $path] {
		    if [file isdirectory $path] {
			if [info exists g_loadedModulesGeneric($mod)] {
			    set ModulesVersion $g_loadedModulesGeneric($mod)
			} elseif [file exists "$path/.version"] {
			    source "$path/.version"
			} else {
			    set ModulesVersion [file tail [lindex [glob "$path/*"] 0]]
			}
			set path "$path/$ModulesVersion"
			set mod "$mod/$ModulesVersion"
		    }
		    
		    if [file isfile $path] {
			return [list $path $mod]
		    }
		} else {
		    error "$path not readable"
		}
	    }
	}
	error "Module $mod not found on \$MODULEPATH.\nHINT: Use 'module use ...' to add to search path."
    } else {
	error "\$MODULEPATH not defined"
    }
}

proc renderSettings {} {
    global env g_Aliases g_shellType g_shell
    global g_stateEnvVars g_stateAliases
    global g_newXResources g_delXResources


    set iattempt 0
    set f ""
    while {$iattempt < 100 && $f == ""} {
	set tmpfile [format "/tmp/modulescript_%d_%d" [pid] $iattempt]
	set f [open  $tmpfile "w"]
	incr iattempt
    }
    
    if {$f == ""} {
	error "Could not open a temporary file in /tmp/modulescript_* !"
    } else {

# preliminaries
	switch $g_shellType {
	    python {
		puts $f "import os";
	    }
	}

# new environment variables
	foreach var [array names g_stateEnvVars] {
	    if {$g_stateEnvVars($var) == "new"} {
		switch $g_shellType {
		    csh {
			set val [doubleQuoteEscaped $env($var)]
			puts $f "setenv $var \"$val\""
		    }
		    sh {
			set val [doubleQuoteEscaped $env($var)]
			puts $f "$var=\"$val\"; export $var"
		    }
		    perl {
			set val [doubleQuoteEscaped $env($var)]
			puts $f "\$ENV{$var}=\"$val\";"
		    }
		    python {
			set val [singleQuoteEscaped $env($var)]
			puts $f "os.environ\['$var'\] = '$val'"
		    }
		}
		
	    }
	}
# new aliases
	foreach var [array names g_newAliases] {
	    switch $g_shellType {
		csh {
		    set val [multiEscaped $g_newAliases($var)]
		    puts $f "alias $var $val"
		}
		sh {
		    set val $g_newAliases($var)
		    puts $f "$var () {\n$val\n}"
		}
	    }
	}

# obsolete (deleted) env vars	
	foreach var [array names g_stateEnvVars] {
	    if {$g_stateEnvVars($var) == "del"} {
		switch $g_shellType {
		    csh {
			puts $f "unsetenv $var"
		    }
		    sh {
			puts $f "unset $var"
		    }
		    perl {
			puts $f "delete \$ENV{$var};"
		    }
		    python {
# this is not correct, but I'm not a python programmer
			puts $f "os.environ\['$var'\] = ''"
		    }
		}
	    }
	}
# obsolete aliases
	foreach var [array names g_delAliases] {
	    switch $g_shellType {
		csh {
		    puts $f "unalias $var"
		}
		sh {
		    if {$g_shell == "zsh"} {
			puts $f "unfunction $var"
		    } else {
			puts $f "unset $var"
		    }
		}
	    }
	}

# new x resources
	if {[array size g_newXResources] > 0} {
	    set xrdb [findExecutable xrdb]
	    foreach var [array names g_newXResources] {
		set val $g_newXResources($var)
		if { $val == "" } {
		    switch -regexp $g_shellType {
			{^(csh|sh)$} {
			    if [ file exists $var] {
				puts $f "$xrdb -merge $var"
			    } else {
				puts $f "$xrdb -merge <<EOF"
				puts $f "$var"
				puts $f "EOF"
			    }
			}
			perl {
			    if [ file isfile $var] {
				puts $f "system(\"$xrdb -merge $var\");"
			    } else {
				puts $f "open(XRDB,\"|$xrdb -merge\");"
				set var [doubleQuoteEscaped $var]
				puts $f "print XRDB \"$var\\n\";"
				puts $f "close XRDB;"
			    }
			} 
			python {
			    if [ file isfile $var] {
				puts $f "os.popen('$xrdb -merge $var');"
			    } else {
				set var [singleQuoteEscaped $var]
				puts $f "os.popen('$xrdb -merge').write('$var')"
			    }
			}
		    }
		} else {
		    switch -regexp $g_shellType {
			{^(csh|sh)$} {
			    puts $f "$xrdb -merge <<EOF"
			    puts $f "$var: $val"
			    puts $f "EOF"
			}
			perl {
			    puts $f "open(XRDB,\"|$xrdb -merge\");"
			    set var [doubleQuoteEscaped $var]
			    set val [doubleQuoteEscaped $val]
			    puts $f "print XRDB \"$var: $val\\n\";"
			    puts $f "close XRDB;"
			}
			python {
			    set var [singleQuoteEscaped $var]
			    set val [singleQuoteEscaped $val]
			    puts $f "os.popen('$xrdb -merge').write('$var: $val')"
			}
		    }
		}
	    }
	}
	if {[array size g_delXResources] > 0} {
	    set xrdb [findExecutable xrdb]
	    foreach var [array names g_delXResources] {
		if {$val == ""} {
		    # do nothing
		} else {
		    puts $f "xrdb -remove <<EOF"
		    puts $f "$var:"
		    puts $f "EOF"
		}
	    }
	}
	switch $g_shellType {
	    csh {
		puts $f "/bin/rm -f $tmpfile"
	    }
	    sh {
		puts $f "/bin/rm -f $tmpfile"
	    }
	    perl {
		puts $f "unlink(\"$tmpfile\");"
	    }
	    python {
		puts $f "os.unlink('$tmpfile')"
	    }
	}

	close $f

	switch $g_shellType {
	    csh {
		puts "source $tmpfile"
	    }
	    sh {
		puts ". $tmpfile"
	    }
	    perl {
		puts "do \"$tmpfile\";"
	    }
	    python {
# this is not correct
		puts "exec '$tmpfile'"
	    }
	}
    }
}

proc cacheCurrentModules {} {
    global g_loadedModules g_loadedModulesGeneric env

# mark specific as well as generic modules as loaded
    if [info exists env(LOADEDMODULES)] {
	foreach mod [split $env(LOADEDMODULES) ":"] {
	    set g_loadedModules($mod) 1
	    set g_loadedModulesGeneric([file dirname $mod]) [file tail $mod]
	}
    }
}

proc spaceEscaped {text} {
    regsub -all " " $text "\\ " text
    return $text
}

proc multiEscaped {text} {
    regsub -all {["'; ]} $text {\\\0} text 
#"
    return $text
}

proc doubleQuoteEscaped {text} {
    regsub -all "\"" $text "\\\"" text
    return $text
}

proc singleQuoteEscaped {text} {
    regsub -all "\'" $text "\\\'" text
    return $text
}

proc findExecutable {cmd} {
    foreach dir {/usr/X11R6/bin /usr/openwin/bin /usr/bin/X11} {
	if [file executable "$dir/$cmd"] {
	    return "$dir/$cmd"
	}
    }
    return $cmd
}

########################################################################
# command line commands

proc cmdModuleList {} {
    global env

    if [info exists env(LOADEDMODULES)] {
	set list [split $env(LOADEDMODULES) ":"]
	set max 0
	foreach mod $list {
	    if {[string length $mod] > $max} {
		set max [string length $mod]
	    }
	}
# save room for numbers and spacing: 2 digits + ) + space + space
	incr max 1
	set cols [expr int(79/($max+4))]
	set lines [expr int(([llength $list] -1)/ $cols) +1]
	for {set i 0} { $i < $lines} {incr i} {
	    for {set col 0} {$col < $cols } { incr col} {
		set index [expr $col * $lines + $i]
		set mod [lindex $list $index]
		if {$mod != ""} {
		    set mod [format "%2d) %-${max}s" $index $mod]
		    puts -nonewline stderr $mod
		}
	    }
	    puts stderr ""
	}
    }
}

proc cmdModuleDisplay {mod} {
    global env tcl_version

    set g_mode "display"
    catch {
	set modfile [getPathToModule $mod]
	set currentModule [lindex $modfile 1]
	set modfile [lindex [getPathToModule $mod] 0]
	puts stderr "-------------------------------------------------------------------"
	puts stderr "$modfile:\n"
	source $modfile
	puts stderr "-------------------------------------------------------------------"
	set junk ""
    } errMsg
    if {$errMsg != ""} {
	puts stderr "ERROR: module display $mod failed. $errMsg"
    }
}

proc cmdModuleWhatIs {{mod {}}} {
    cmdModuleSearch $mod {}
}

proc cmdModuleApropos {{search {}}} {
    cmdModuleSearch {} $search
}

proc cmdModuleSearch {{mod {}} {search {}} } {
    global env tcl_version ignoreDir
    
    if {$mod == ""} {
	set mod  "*"
    }
    foreach dir [split $env(MODULEPATH) ":"] {
	if [file isdirectory $dir] {
	    puts stderr "---- $dir ---- "
	    cd $dir
	    array set availHash {}
	    if [file isfile $mod] {
		set availHash($mod) 1
	    }
 	    foreach file [glob -nocomplain "$mod/*"] {
		set pieces [split $file "/"]
		set ok 1
		foreach piece $pieces {
		    if [info exists ignoreDir($piece)] {
			set ok 0
		    }
		}
		if {$ok == 1} {
		    set availHash($file) 1
		}
	    }
	    foreach mod2 [lsort [array names availHash]] {
		set g_whatis ""
		catch {
		    set modfile [getPathToModule $mod2]
		    set currentModule [lindex $modfile 1]
		    set modfile       [lindex $modfile 0]
		    set g_mode "whatis"
		    source $modfile
		    set junk ""
		} errmsg
		if {$search =="" || [regexp -nocase $search $g_whatis]} {
		    puts stderr [format "%20s: %s" $mod2 $g_whatis]
		}
	    }
	    unset availHash
	}
    }
}

proc cmdModuleSwitch {old {new {}}} {
    if {$new == ""} {
	set new $old
	if {[file dirname $old] != "."} {
	    set old [file dirname $old]
	}
    }

    cmdModuleUnload $old
    cmdModuleLoad $new
}

proc cmdModuleLoad {args} {
    global env tcl_version g_loadedModules g_loadedModulesGeneric g_force
 
    foreach mod $args {
	catch {
	    set modfile [getPathToModule $mod]
	    set currentModule [lindex $modfile 1]
	    set modfile       [lindex $modfile 0]

	    if { $g_force || ! [ info exists g_loadedModules($currentModule)]} {
		set g_mode "load"

		append-path LOADEDMODULES $currentModule
		set g_loadedModules($currentModule) 1
		set g_loadedModulesGeneric([file dirname $mod]) [file tail $currentModule]

		source $modfile
	    }
	    set junk ""
	} errMsg
	
        if {$errMsg != ""} {
	    puts stderr "ERROR: module load $mod failed. $errMsg"
	}
    }
}

proc cmdModuleUnload {args} {
    global env tcl_version g_loadedModules g_loadedModulesGeneric
 
    foreach mod $args {
	catch {
	    catch {
		set modfile [getPathToModule $mod]
		set currentModule [lindex $modfile 1]
		set modfile       [lindex $modfile 0]
		set junk ""
	    } moderr
	    if { $moderr == ""} {
		if [ info exists g_loadedModules($currentModule)] {
		    set g_mode "unload"
		    source $modfile
		    unload-path LOADEDMODULES $currentModule
		    unset g_loadedModules($currentModule)
		    if [info exists g_loadedModulesGeneric([file dirname $currentModule])] {
			unset g_loadedModulesGeneric([file dirname $currentModule])
		    }
		}
	    } else {
		puts stderr "WARNING: Could not find module $mod, $moderr\n"
		if [info exists g_loadedModulesGeneric($mod) ] {
		    set mod "$mod/$g_loadedModulesGeneric($mod)"
		}
		unload-path LOADEDMODULES $mod
		if [info exists g_loadedModules($mod) ] {
		    unset g_loadedModules($mod)
		}
		if [info exists g_loadedModulesGeneric([file dirname $mod])] {
		    unset g_loadedModulesGeneric([file dirname $mod])
		}
	    }
	    set junk ""
	} errMsg
	if {$errMsg != ""} {
	    puts stderr "ERROR: module unload $mod failed. $errMsg"
	}
    }
}


proc cmdModulePurge {} {
    global env
    
    if [info exists env(LOADEDMODULES)] {
	set list [split $env(LOADEDMODULES) ":"] 
	eval cmdModuleUnload $list
    }
}

proc cmdModuleReload {} {
    global env
    
    if [info exists env(LOADEDMODULES)] {
	set list [split $env(LOADEDMODULES) ":"] 
	set rlist {}
	for {set i [expr [llength $list] - 1]} {$i>=0} {incr i -1} {
	    lappend rlist [lindex $list $i]
	}
	eval cmdModuleUnload $rlist
	eval cmdModuleLoad $list
    }
}


proc cmdModuleAvail { {mod {}}} {
    global env ignoreDir

    if {$mod == ""} {
	set mod  "*"
    }
    foreach dir [split $env(MODULEPATH) ":"] {
	if [file isdirectory $dir] {
	    puts stderr "---- $dir ---- "
	    cd $dir
	    array set availHash {}
	    if [file isfile $mod] {
		set availHash($mod) 1
	    }
 	    foreach file [glob -nocomplain "$mod/*"] {

		set pieces [split $file "/"]
		set ok 1
		foreach piece $pieces {
		    if [info exists ignoreDir($piece)] {
			set ok 0
		    }
		}
		if {$ok == 1} {
		    set availHash($file) 1
		}
	    }
	    set list [lsort [array names availHash]]

	    set max 0
	    foreach mod2 $list {
		if {[string length $mod2] > $max} {
		    set max [string length $mod2]
		}
	    }
	    incr max 1
	    set cols [expr int(79/($max))]
	    set lines [expr int(([llength $list] -1)/ $cols) +1]
	    for {set i 0} { $i < $lines} {incr i} {
		for {set col 0} {$col < $cols } { incr col} {
		    set index [expr $col * $lines + $i]
		    set mod2 [lindex $list $index]
		    if {$mod2 != ""} {
			set mod2 [format "%-${max}s" $mod2]
			puts -nonewline stderr $mod2
		    }
		}
		puts stderr ""
	    }
	    
	    unset availHash
	}
    }
}

proc cmdModuleUse {args} {

    set g_mode "load"
    foreach path $args {
	if [file isdirectory $path] {
	    prepend-path MODULEPATH $path
	} else {
	    error "Directory $path does not exist"
	}
    }
}

proc cmdModuleUnuse {args} {

    set g_mode "unload"
    foreach path $args {
	if [file isdirectory $path] {
	    unload-path MODULEPATH $path
	} else {
	    error "Directory $path does not exist"
	}
    }
}

proc cmdModuleDebug {} {
    global env

    foreach var [array names env] {
	set sharevar "${var}_modshare"
	if [info exists env($sharevar)] {
	    array set countarr [split $env($sharevar) ":"]
	    foreach path [array names countarr] {
		puts stderr "$var\t$path\t$countarr($path)"
	    }
	    unset countarr
	}
    }
    foreach dir  [split $env(PATH) ":"] {
	foreach file [glob -nocomplain "$dir/*"] {
	    if [file exists $file] {
		set exec [file tail $file]
		if [info exists execcount($exec)] {
		    lappend execcount($exec) $file
#		    puts stderr "conflict1: $execcount($exec)"
#		    puts stderr [llength $execcount($exec)]
		} else {
		    lappend execcount($exec) $file
#		    set execcount($exec) $file
		}
	    }
	}
    }
    foreach file [lsort [array names execcount]] {
	if {[llength $execcount($file)] > 1} {
	    puts stderr "$file:\t$execcount($file)"
	}
    }
}

########################################################################
# main program

global g_shellType

set g_shell [lindex $argv 0]
set command [lindex $argv 1]
set argv [lreplace $argv 0 1] 
set g_force 0

switch -regexp $g_shell {
    ^(sh|bash|ksh|zsh)$ {
	set g_shellType sh
    }
    ^(csh|tcsh)$ {
	set g_shellType csh
    }
    ^(perl)$ {
	set g_shellType perl
    }
    ^(python)$ {
	set g_shellType python
    }
    . {
	error "bad shell $g_shell"
    }
}

cacheCurrentModules

catch {
    switch -regexp $command {
	{^av} {
	    if {$argv != ""} {
		foreach arg $argv {
		    cmdModuleAvail $arg
		}
	    } else {
		cmdModuleAvail
	    }
	}
	{^li} {
	    cmdModuleList
	}
	{^(di|show)} {
	    foreach arg $argv {
		cmdModuleDisplay $arg
	    }
	}
	{^(add|load)} {
	    eval cmdModuleLoad $argv
	    renderSettings
	}
	{^pu} {
	    cmdModulePurge
	    renderSettings
	}
	{^sw} {
	    eval cmdModuleSwitch $argv
	    renderSettings
	}
	{^(rm|unload)} {
	    eval cmdModuleUnload $argv
	    renderSettings
	}
	{^use$} {
	    eval cmdModuleUse $argv
	    renderSettings
	}
	{^unuse$} {
	    eval cmdModuleUnuse $argv
	    renderSettings
	}
	{^wh} {
	    if {$argv != ""} {
		foreach arg $argv {
		    cmdModuleWhatIs $arg
		}
	    } else {
		cmdModuleWhatIs
	    }
	}
	{^apropos$} {
	    eval cmdModuleApropos $argv
	}
	{^debug$} {
	    eval cmdModuleDebug
	}
	{^rel} {
	    cmdModuleReload
	    renderSettings
	}
	default {
	    puts stderr {
		ModulesTcl 0.95 :
		Available Commands and Usage:
		 add|load              modulefile [modulefile ...]
		 rm|unload             modulefile [modulefile ...]
                 reload
		 switch|swap           modulefile1 modulefile2
		 display|show          modulefile [modulefile ...]
		 avail                 [modulefile [modulefile ...]]
		 use                   dir [dir ...]
		 unuse                 dir [dir ...]
		 purge
		 list
		 whatis                [modulefile [modulefile ...]]
		 apropos|keyword       string
	    }
	}
	
    }
} errMsg

if {$errMsg != ""} {
    puts stderr "ERROR: $errMsg"
}


 

