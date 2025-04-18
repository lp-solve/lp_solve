// Generated by builder.sce - Do Not Edit
mode(-1);
p = pwd();
if MSDOS then
    win64_ = %F;
    execstr('win64_=win64()','errcatch');
    p = strsubst(p,'\','/');
    bindir = 'win';
    if win64_ then
        bindir = bindir + '64';
    else
        bindir = bindir + '32';
    end
    libs = p + '/../../../lpsolve55/bin/' + bindir + '/lpsolve55'
    libsext = '.dll';
else
    bindir = 'win32';
    libs = 'liblpsolve55';
    libsext = '.so'
end
link(libs + libsext);
sclpsolvelib = 'sclpsolve';
if ~MSDOS then
    if fileinfo(p + '/bin/' + bindir + '/' + sclpsolvelib + libsext) == [] then
        sclpsolvelib = 'libsclpsolve';
    end
end
addinter(p + '/bin/' + bindir + '/' + sclpsolvelib + libsext,sclpsolvelib,['sclpsolve';]);
genlib('lpmexlib',p + '/macros');
prot=funcprot();
funcprot(0);
formatman(p + '/man');
funcprot(prot);
clear p prot win64_ sclpsolvelib bindir;
