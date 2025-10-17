declare global
{
    interface FileHandle
    {
	    read(size:number,position:number): Promise<string>;
        write(buf:string,pos?:number): Promise<void>;
        close(): Promise<void>;
    }
    interface fsModule
    {
        O_RDONLY: number;
        O_WRONLY: number;
        O_RDWR: number;
        O_APPEND: number;
        O_CREAT: number;
        O_EXCL: number;
        O_TRUNC: number;
        O_BINARY: number;
        O_TEXT: number;
        S_IFIFO: number;
        S_IFCHR: number;
        S_IFDIR: number;
        S_IFBLK: number;
        S_IFREG: number;
        S_IFSOCK: number;
        S_IFLNK: number;
        S_ISGID: number;
        S_ISUID: number;
        access(path:string): Promise<void>;
        appendFile(path:string,buf:string): Promise<void>;
        chmod(path:string,mode:number): Promise<void>;
        chown(path: string, uid:number,gid:number): Promise<void>;
        copyFile(src: string,dst:string);
        mkdir(path:string, mode:number);
        mkdtemp(path:string): Promise<string>;
        readFile(path:string):Promise<string>;
        readdir(path:string): Promise<Array>;
        rmdir(path:string): Promise<void>;
        rename(p1:string,p2:string): Promise<void>;
        unlink(path: string): Promise<void>;
        stat(path:string): Promise<Object>;
        symlink(p1:string,p2:string): Promise<void>;
        truncate(path:string,mode:number): Promise<void>;
        utimes(path:string,a:number,m:number): Promise<void>;
        writeFile(path:string, buf:string):Promise<void>;
        open(path:string, flags:number, mode?:number): Promise<FileHandle>;
        
    }
    // const fs: fsModule;

}
export {  };