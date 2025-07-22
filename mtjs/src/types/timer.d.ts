// timer.d.ts
export class Timeout {
  public  unref(): void ;
}

declare global {
  /**
   * Вызывает функцию `callback` через указанное количество миллисекунд `ms`.
   * Возвращает числовой идентификатор, который можно передать в `clearTimeout`.
   * Дополнительные аргументы `...args` пробрасываются в `callback`.
   *
   * @param callback Функция, которая будет вызвана по таймеру.
   * @param ms Время в миллисекундах до выполнения.
   * @param args Дополнительные аргументы для `callback`.
   * @returns Числовой идентификатор таймера.
   */
  function setTimeout(
    callback: (...args: any[]) => void,
    ms: number,
    ...args: any[]
  ): Timeout;

  /**
   * Останавливает таймер, заданный через `setTimeout` по его числовому идентификатору.
   * @param timerId Идентификатор таймера, возвращённый `setTimeout`.
   */
  function clearTimeout(timerId: Timeout): void;

  /**
   * Вызывает функцию `callback` каждые `ms` миллисекунд,
   * пока этот таймер не будет остановлен через `clearInterval`.
   * Возвращает числовой идентификатор, который можно передать в `clearInterval`.
   * @param callback Функция, которая будет вызвана по таймеру.
   * @param ms Интервал в миллисекундах между вызовами.
   * @param args Дополнительные аргументы для `callback`.
   * @returns Числовой идентификатор таймера.
   */
  function setInterval(
    callback: (...args: any[]) => void,
    ms: number,
    ...args: any[]
  ): Timeout;

  /**
   * Останавливает повторяющийся таймер, заданный через `setInterval`,
   * по его числовому идентификатору.
   * @param timerId Идентификатор таймера, возвращённый `setInterval`.
   */
  function clearInterval(timerId: Timeout): void;
}

export {};
