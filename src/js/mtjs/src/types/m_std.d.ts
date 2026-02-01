// quickjs-std.d.ts

// Тип для файлового дескриптора
type FileHandle = number;

// Тип для режимов открытия файла
type FileMode = "r" | "w" | "a" | "r+" | "w+" | "a+" | string;

// Описание модуля std
declare module "std" {
  // Функции для работы с файлами
  function fopen(path: string, mode: FileMode): FileHandle;
  function fclose(file: FileHandle): number;
  function fputs(str: string, file: FileHandle): number;
  function fputc(char: number, file: FileHandle): number;
  function fread(buffer: ArrayBuffer, size: number, n: number, file: FileHandle): number;
  function fwrite(buffer: ArrayBuffer, size: number, n: number, file: FileHandle): number;
  function feof(file: FileHandle): boolean;
  function ferror(file: FileHandle): number;
  function getc(file: FileHandle): number;
  function gets(maxSize: number, file: FileHandle): string | null;
  function fflush(file: FileHandle): number;

  // Функции для работы с процессами
  function popen(command: string, mode: "r" | "w"): FileHandle;
  function pclose(file: FileHandle): number;
  function system(command: string): number;

  // Работа с окружением
  function getenv(name: string): string | null;
  function setenv(name: string, value: string, overwrite: number): number;
  function unsetenv(name: string): number;

  // Утилиты
  function exit(status: number): never;
  function gc(): void;
  function evalScript(script: string, options?: { backtrace?: boolean }): any;
  function loadScript(path: string): any;
  function sprintf(format: string, ...args: any[]): string;
  function printf(format: string, ...args: any[]): number;
  function strerror(errno: number): string;

  // Константы
  const stdin: FileHandle;
  const stdout: FileHandle;
  const stderr: FileHandle;

  // Объект модуля std
  const std: {
    fopen: typeof fopen;
    fclose: typeof fclose;
    fputs: typeof fputs;
    fputc: typeof fputc;
    fread: typeof fread;
    fwrite: typeof fwrite;
    feof: typeof feof;
    ferror: typeof ferror;
    getc: typeof getc;
    gets: typeof gets;
    fflush: typeof fflush;
    popen: typeof popen;
    pclose: typeof pclose;
    system: typeof system;
    getenv: typeof getenv;
    setenv: typeof setenv;
    unsetenv: typeof unsetenv;
    exit: typeof exit;
    gc: typeof gc;
    evalScript: typeof evalScript;
    loadScript: typeof loadScript;
    sprintf: typeof sprintf;
    printf: typeof printf;
    strerror: typeof strerror;
    stdin: typeof stdin;
    stdout: typeof stdout;
    stderr: typeof stderr;
  };

  export = std;
}
