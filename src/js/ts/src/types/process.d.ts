interface Process
{
    env(): { [key: string]: string | undefined };
    readonly argv: string[];
    readonly arch: string;
    readonly platform: string;
    readonly version: string;
    readonly versions: { [key: string]: string | undefined };
    exit(code?: number): never;
    abort(): never;
    cwd(): string;
    chdir(directory: string): void;
    uptime(): number;
    hrtime(): [number, number];
    readonly title:string;
    peakMemoryUsage():number;
    getuid():number;
    getgid():number;
}
declare global
{

    const process: Process;

}
export {  };