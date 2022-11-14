// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
for (int i = 1; i <=1000; ++i){
  int number;
  number += i.count(3);
}
cout << number << endl;

// Закомитьте изменения и отправьте их в свой репозиторий.
git add .
  git commit -m "first commit to the project"
  git push
