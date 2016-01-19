//
// this is my personal .xrefrc file
//

// options commons for all projects
-htmlroot=/home/vittek/HTML
-htmltab=4

// now options per project

[/home/vittek/ctmp]

[/home/vittek/tmp]

-cplusplus

[/home/vittek/xref-any]

-DORIGINAL_MAIN
-D__QNX__
-DZERO
-DPRESERVE_C_ARGS
-DCOMPLETION
-DGENERATE
-DCXREF 
-DDEBUG
-DGNU_LINUX
-DWAT_QNX
-DCCC_ALLOWED
-DYACC_ALLOWED
-DJAVA_UNZIP_ALLOWED
-brief                      // produce brief cross-ference file
-refs /home/vittek/XXref    // absolute path stronly recommended
-refnum=10



[/home/vittek/elan]

-cplusplus
-brief                      // produce brief cross-ference file
-refs /home/vittek/XElan    // absolute path stronly recommended
-refnum=10




[/home/vittek/solve]

-DZERO
-DTRAIL_UNI
-DDEBUG				// a macro definition
-DMOJAVERZIA=2.7	// macro with body
-I /usr/lib/gcc-lib/i386-redhat-linux/2.7.2.3/include
-brief
-refs /home/vittek/XSol




[/home/vittek/java/solver]

-brief
-refs /home/vittek/XJSol

[/home/vittek/java/SK/gnome]

-brief
-refs /home/vittek/Xgnome

[/home/vittek/java/pierrot]

-brief
-refs /home/vittek/Xpierrot




[/home/vittek/soft/coda-4.7.1]

-I /home/vittek/soft/coda-4.7.1/lib-src/mlwp
-I /home/vittek/soft/coda-4.7.1/coda-src/util
-I /home/vittek/soft/coda-4.7.1/coda-src/rpc2
-I /home/vittek/soft/coda-4.7.1/kernel-src/vfs/bsd44
-I /home/vittek/soft/coda-4.7.1/lib-src/base
-I /home/vittek/soft/coda-4.7.1/rvm-src/rvm
-I /home/vittek/soft/coda-4.7.1/rvm-src/rds
-I /home/vittek/soft/coda-4.7.1/coda-src/dir
-I /home/vittek/soft/coda-4.7.1/coda-src/vicedep
-I /home/vittek/soft/coda-4.7.1/coda-src/libal
-I /home/vittek/soft/coda-4.7.1/lib-src/special-includes
-I /home/vittek/soft/coda-4.7.1/coda-src/vv
-I /home/vittek/soft/coda-4.7.1/coda-src/vol
-I /home/vittek/soft/coda-4.7.1/coda-src/partition
-I /home/vittek/soft/coda-4.7.1/coda-src/kerndep
-I /home/vittek/soft/coda-4.7.1/kernel-src/vfs/include
-I /home/vittek/soft/coda-4.7.1/kernel-src/vfs/bsd44/cfs
-brief
-refs /home/vittek/XCoda
-refnum=100

[/home/vittek/soft/apache_1.3.2]

-I /home/vittek/soft/apache_1.3.2/src/include
-I /home/vittek/soft/apache_1.3.2/src/os/unix
-brief
-refs /home/vittek/XApache
-refnum=100

[/home/vittek/soft/linux]

-D__KERNEL__
-I /home/vittek/soft/linux/include
-brief
-refs /home/vittek/XLinux
-refnum=100

[/home/vittek/ctmp/ltest]

-D__KERNEL__
-I /home/vittek/soft/linux/include
-I /home/vittek/soft/linux/drivers/scsi
-brief
-refs /home/vittek/ctmp/ltest/Xrefs

[/home/vittek/soft/qnx/HDO]

-I /home/vittek/soft/qnx/HDO/h
-I /home/vittek/soft/qnx/qlib/include
-I /home/vittek/soft/qnx/windows/include
-I /home/vittek/soft/qnx/usr/include
-brief
-refs /home/vittek/XHDO
-refnum=10


















