///typescript
// quickjs-os.d.ts

// Тип для файлового дескриптора
type FileHandle = number;

// Тип для режимов открытия файла
type FileMode = "r" | "w" | "a" | "r+" | "w+" | "a+" | string;

// Тип для флагов открытия файла (O_RDONLY, O_WRONLY, и т.д.)
type OpenFlags = number;

// Тип для структуры stat
interface Stat {
  dev: number;
  ino: number;
  mode: number;
  nlink: number;
  uid: number;
  gid: number;
  rdev: number;
  size: number;
  atime: number;
  mtime: number;
  ctime: number;
}

// Тип для структуры dirent
interface Dirent {
  name: string;
  isBlockDevice: boolean;
  isCharacterDevice: boolean;
  isDirectory: boolean;
  isFIFO: boolean;
  isRegularFile: boolean;
  isSocket: boolean;
  isSymbolicLink: boolean;
}

// Тип для сетевого адреса
interface SockAddr {
  family: number;
  addr: string;
  port: number;
}

// Описание модуля os
declare module "os" {
  // Функции для работы с файлами
  function open(path: string, flags: OpenFlags, mode?: number): FileHandle;
  function close(fd: FileHandle): number;
  function read(fd: FileHandle, buffer: ArrayBuffer, offset: number, length: number): number;
  function write(fd: FileHandle, buffer: ArrayBuffer, offset: number, length: number): number;
  function stat(path: string): Stat;
  function lstat(path: string): Stat;
  function fstat(fd: FileHandle): Stat;
  function unlink(path: string): number;
  function rename(oldPath: string, newPath: string): number;
  function mkdir(path: string, mode?: number): number;
  function rmdir(path: string): number;
  function readdir(path: string): Dirent[];

  // Функции для работы с процессами
  function exec(args: string[]): number;
  function waitpid(pid: number, options: number): [number, number];
  function kill(pid: number, sig: number): number;

  // Функции для работы с сетью
  function socket(family: number, type: number, protocol: number): FileHandle;
  function connect(fd: FileHandle, addr: SockAddr): number;
  function bind(fd: FileHandle, addr: SockAddr): number;
  function listen(fd: FileHandle, backlog: number): number;
  function accept(fd: FileHandle): [FileHandle, SockAddr];
  function send(fd: FileHandle, buffer: ArrayBuffer, offset: number, length: number): number;
  function recv(fd: FileHandle, buffer: ArrayBuffer, offset: number, length: number): number;
  function setsockopt(fd: FileHandle, level: number, option: number, value: number): number;
  function getsockopt(fd: FileHandle, level: number, option: number): number;

  // Функции для работы с временем
  function setTimeout(callback: () => void, delay: number): number;
  function clearTimeout(timer: number): void;
  function setInterval(callback: () => void, interval: number): number;
  function clearInterval(timer: number): void;
  function now(): number;

  // Функции для сигналов
  function signal(sig: number, handler: ((sig: number) => void) | number): void;

  // Утилиты
  function getcwd(): string;
  function chdir(path: string): number;
  function platform(): string;
  function getpid(): number;
  function getppid(): number;
  function sleep(ms: number): void;

  // Константы
  const O_RDONLY: number;
  const O_WRONLY: number;
  const O_RDWR: number;
  const O_APPEND: number;
  const O_CREAT: number;
  const O_EXCL: number;
  const O_TRUNC: number;

  const S_IFMT: number;
  const S_IFREG: number;
  const S_IFDIR: number;
  const S_IFCHR: number;
  const S_IFBLK: number;
  const S_IFIFO: number;
  const S_IFLNK: number;
  const S_IFSOCK: number;

  const SIGINT: number;
  const SIGTERM: number;
  const SIGKILL: number;
  // Другие сигналы...

  // Объект модуля os
  const os: {
    open: typeof open;
    close: typeof close;
    read: typeof read;
    write: typeof write;
    stat: typeof stat;
    lstat: typeof lstat;
    fstat: typeof fstat;
    unlink: typeof unlink;
    rename: typeof rename;
    mkdir: typeof mkdir;
    rmdir: typeof rmdir;
    readdir: typeof readdir;
    exec: typeof exec;
    waitpid: typeof waitpid;
    kill: typeof kill;
    socket: typeof socket;
    connect: typeof connect;
    bind: typeof bind;
    listen: typeof listen;
    accept: typeof accept;
    send: typeof send;
    recv: typeof recv;
    setsockopt: typeof setsockopt;
    getsockopt: typeof getsockopt;
    setTimeout: typeof setTimeout;
    clearTimeout: typeof clearTimeout;
    setInterval: typeof setInterval;
    clearInterval: typeof clearInterval;
    now: typeof now;
    getpdb: typeof getpdb;
    chdir: typeof chdir;
    platform: typeof platform;
    getpid: typeof getpid;
    getppid: typeof getppid;
    sleep: typeof sleep;
    O_RDONLY: typeof O_RDONLY;
    O_WRONLY: typeof O_WRONLY;
    O_RDWR: typeof O_RDWR;
    O_APPEND: typeof O_APPEND;
    O_CREAT: typeof O_CREAT;
    O_EXCL: typeof O_EXCL;
    O_TRUNC: typeof O_TRUNC;
    S_IFMT: typeof S_IFMT;
    S_IFREG: typeof S_IFREG;
    S_IFDIR: typeof S_IFDIR;
    S_IFCHR: typeof S_IFCHR;
    S_IFBLK: typeof S_IFBLK;
    S_IFIFO: typeof S_IFIFO;
    S_IFLNK: typeof S_IFLNK;
    S_IFSOCK: typeof S_IFSOCK;
    SIGINT: typeof SIGINT;
    SIGTERM: typeof SIGTERM;
    SIGKILL: typeof SIGKILL;
  };

  export = os;
}
