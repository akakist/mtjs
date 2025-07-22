

  interface Pipe {
    /**
     * Выполняет команду через pipe и вызывает callback со строкой (весь вывод команды).
     * @param path Путь или команда для исполнения.
     * @param callback Функция обратного вызова, получающая строку вывода.
     */
    read(path: string, output: Stream): Promise<void>;

    /**
     * Выполняет команду через pipe, читая вывод построчно, и вызывает callback для каждой строки.
     * @param path Путь или команда для исполнения.
     * @param callback Функция обратного вызова, получающая очередную строку вывода.
     */
    read_lines(path: string, output: Stream): Promise<void>;

  }


  interface STDIN {
    /**
     * Читает стандартный ввод построчно, вызывая callback для каждой полученной строки.
     * @param callback Функция обратного вызова, принимающая очередную строку ввода.
     */
    read_lines(output:Stream): Promise<void>;

    /**
     * Читает стандартный ввод полностью, затем вызывает callback с полученным содержимым.
     * @param callback Функция обратного вызова, получающая строку с полным содержимым ввода.
     */
    toString(callback: (content: string) => void): void;
  }

