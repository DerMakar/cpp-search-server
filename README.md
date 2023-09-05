# SearchServer
Поисковый сервер с функцией загрузки документов и поиска в базе релевантных запросу файлов. Документам присваивается статус и пользовательский рейтинг, что влияет на результаты поиска. Пользователь может указывать минус-слова, чтобы исключить из подборки поиска ненужные файлы. В поиковых алгоритмах добавлена функция параллельной работы с пачками документов, что обеспечивает быструю подборку релевантных файлов.

# Использование
1. Пример использования - в main.сpp
2. При создании базы можно указать коллекцию стоп-слов, которые не будут участвовать в поиске
3. Метод AddDocument позволяет наполнить базу
4. Метод FindTopDocuments с разными настройками позволит подобрать нужную коллекцию релевантных документов
5. Функция PrintDocument выведет id-документов и информацию об их релевантности.

# Системные требования
1. С++17

# Планы по добработке
1. Ускорение работы функции ProcessQueries
2. Создание дескоптного приложения

# Примечание
Первый тренировочный проект ЯндексПрактикума для отработки основных компонент С++: разработки классов, распределения кода по файлам и ассинхронных вычислений.
