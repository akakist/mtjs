declare global
{

    interface telnetRequest
    {
        readonly command:string;
        readonly path:string;
        print(...args: any[]):void;
    }
    interface telnetModule
    {
        register_command(directory:string, command:string,help:string):void;
        set_callback(callback: (data:telnetRequest) => void): void;
        listen(addr:string, deviceName:string): void;

    }
    const telnet: telnetModule;

}
export {  };