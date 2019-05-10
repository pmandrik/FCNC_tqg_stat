
Скопировать tree_to_hists.C, analyse.sh и create_card.py в локалбную директори.
Перед запуском отредактировать tree_to_hists.C:
  * нужно добавить релиз RELEASE, который также будет использоваться при вызовах в analyse.sh и указать путь до используемых файлов после мензуры в строчках https://github.com/pmandrik/FCNC_tqg_stat/blob/master/tree_to_hists.C#L38
  * в строчках https://github.com/pmandrik/FCNC_tqg_stat/blob/83ff99b6a1b674e36dba2fb74704786db26a5218/tree_to_hists.C#L106
    и https://github.com/pmandrik/FCNC_tqg_stat/blob/master/tree_to_hists.C#L128, https://github.com/pmandrik/FCNC_tqg_stat/blob/master/tree_to_hists.C#L300 указать название переменной сети КХД
  * в стрчоках https://github.com/pmandrik/FCNC_tqg_stat/blob/master/tree_to_hists.C#L142 и https://github.com/pmandrik/FCNC_tqg_stat/blob/master/tree_to_hists.C#L152 и дальше указать названия весов СМ и FCNC сетей
  * возможно, убрать хардкод перевзвешивания весов в строчках https://github.com/pmandrik/FCNC_tqg_stat/blob/master/tree_to_hists.C#L193

После этого задать переменную release= в analyse.sh и запускать последовательностью команд:
  * ./analyse.sh start - пересоздаёт рабочую директорию
  * ./analyse.sh qcd   - создаёт гистограммы с КХД, запускет theta для фита и сохраняет результаты фита - нормализацию КХД и остальных гистограмм
  * ./analyse.sh hists sm - создаёт гистограммы для SM анализа и запускает проверочные скрипты. Запускать с параметрами unmarg_sm и fcnc для других анализов
  * ./analyse.sh sm theta - генерирует карту для theta и запускает её для SM анализа. Затем запускает скрипты для генерации pdf с табличкой и результатами анализа.
  * ./analyse.sh fcnc theta - аналогично для FCNC
выходные файлы распологаются в соответствующих поддиректориях рабочей директории

Так как сейчас файлы mensura доступны на pccms, то запускать построение гистограмм нужно там. Из-за переноса статических библиотек на cvmfs ранее существовали проблемы с запуском theta, поэтому соотвветствующие этапы analyse.sh было необходимо проводить на lxplus7.
