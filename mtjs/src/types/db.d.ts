    export class MysqlOpts {

    host?: string;
    port?: number;
    user?: string;
    password?: string;
    db: string;
    socket?: string;
    };
    export class mysql {
      constructor(opts: MysqlOpts);
      query(sql: string): string;  
    };

    declare global {
        const db: 
        {
            mysql_connect: (opts: MysqlOpts) => mysql;
            mysql: typeof mysql;
        };
    }
