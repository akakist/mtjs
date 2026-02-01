    interface Stream {
        write(...args: string): void;
        end(...args: string): void;
        read(): string;
    };

    export class FileWriteableStream implements Stream {
        constructor(fn:string);
        write(...args: string): void;
        end(...args: string): void;
        read(): string;
    };
    export class FileReadableStream implements Stream {
        constructor(fn:string);
        write(...args: string): void;
        end(...args: string): void;
        read(): string;

    };

    export class ConstReadableStringStream implements Stream {

    };
    export class WriteableStringStream extends Stream {
        write(...args: string): void;
        end(...args: string): void;
        read(): string;

    }
    export class EventEmitterStream extends Stream {
        write(...args: string|Uint8Array): void;
        end(...args: string|Uint8Array): void;
        on(event:string, callback: (output: any) => void): void;
        emit(event:string, data:any):void;
        read(): string;

    }

//    declare global {
        interface StreamModule
        {
            FileWriteableStream: typeof FileWriteableStream;
            FileReadableStream: typeof FileReadableStream;
            ConstReadableStringStream: typeof ConstReadableStringStream;
            WriteableStringStream: typeof WriteableStringStream;
            EventEmitterStream: typeof EventEmitterStream;
        };
//    }
