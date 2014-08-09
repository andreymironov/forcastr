#ifndef FORCASTR
#define FORCASTR

#define DP_ORIGINAL		0
#define DP_CALCULATED	1

#include <math.h>
#include <stdio.h>

namespace Forcastr
{
	#define NULL	0

	//Односвязный список
	struct SLList
	{
		void* pNext;

		SLList()
		{
			pNext = NULL;
		}
	};

	//Структура содержит исходные и предсказанные данные
	struct DataPoint:SLList
	{
		//Абсцисса
		float fX;
		//Ордината
		float fY;
		//Оригинальные ли данные?
		char bIsOriginal;

		DataPoint(float fX, float fY, char bIsOriginal) : fX(fX), fY(fY), bIsOriginal(bIsOriginal)
		{
			bIsOriginal = DP_ORIGINAL;
		}
	};

	//Содержит данные о метрике
	struct Metrics:SLList
	{
		//Окно кандидата
		float fLowerCandidateEdge;
		float fMiddleCandidateEdge;
		float fHigherCandidateEdge;
		//Окно проекции
		float fLowerProjectionEdge;
		float fHigherProjectionEdge;
		//Весовой коэффициент
		float fWeight;
		//Масштаб
		float fScale;
		//Матожидание по кандидату
		float fCandidateMean;
		//Матожидание по проекции
		float fProjectionMean;
	};

	class Forcastr
	{
	public:

	private:
		bool bIsForecasted;				//Была ли уже вызвана функция предсказания
		DataPoint* pdpFirst;			//Первый элемент данных
		DataPoint* pdpLast;				//Последний элемент данных
		long lProjectionStepsCount;		//Шаг окна проекций
		long lCandidateStepsCount;		//Шаг окна кандидата
		long lMetricStepsCount;			//Шаг подсчёта метрики
		Metrics* pmFirst;				//Первый элемент списка метрик
		Metrics* pmLast;				//Последний элемент списка метрик
		FILE* DebugPrint;				//Указатель на файл для дебага

	public:

		//Деструктор
		~Forcastr(void)
		{
			//Удаление односвязного списка DataPoint
			CleanUp(pdpFirst);
			//Удаление односвязного списка метрик
			CleanUp(pmFirst);
		}

		void Initialize(void)
		{
			//По умолчанию не подсчитано
			bIsForecasted = false;

			//Нет данных
			pdpFirst = NULL;
			pdpLast = NULL;

			//Нет метрик
			pmFirst = NULL;
			pmLast = NULL;

			DebugPrint = NULL;

			//Дефлотное количество шагов
			SetUp(10, 10, 10);
		}

		void SetUp(const long& lProjectionStepsCount, const long& lCandidateStepsCount, const long& lMetricStepsCount)
		{
			if(!bIsForecasted)
			{
				this->lProjectionStepsCount = lProjectionStepsCount;
				this->lCandidateStepsCount = lCandidateStepsCount;
				this->lMetricStepsCount = lMetricStepsCount;

				if(DebugPrint) fprintf(DebugPrint, "Projection steps count = %i\nCandidate steps count = %i\nMetric steps count = %i\n", this->lProjectionStepsCount, this->lCandidateStepsCount, this->lMetricStepsCount);
			}
		}

		void SetDebugPrint(FILE* DebugPrint)
		{
			if(!bIsForecasted)
			{
				this->DebugPrint = DebugPrint;
			}
		}

		//Добавляет точку к исходным данным
		bool AddPoint(const float& fX, const float& fY)
		{
			return AddPoint(fX, fY, DP_ORIGINAL);
		}

		//Производит прогнозирование
		void ForecastByEdgeAndSteps(const float& fUpperEdge, const long& uStepsCount)
		{
			if(pdpLast && (fUpperEdge > pdpLast->fX) && (uStepsCount > 0))
				Forecast(fUpperEdge, (fUpperEdge - pdpLast->fX) / (float)(uStepsCount), uStepsCount);
		}
		void ForecastByStepAndSteps(const float& fStepSize, const long& uStepsCount)
		{
			if(pdpLast && (fStepSize > 0) && (uStepsCount > 0))
				Forecast(pdpLast->fX + fStepSize * uStepsCount, fStepSize, uStepsCount);
		}

		//Возвращает значение Y для указанного X по введённым данным
		float GetData(const float& fX) const
		{
			if(pdpFirst)
			{
				if(fX <= pdpFirst->fX)
					return pdpFirst->fY;
				else
				{
					if(fX >= pdpLast->fX)
						return pdpLast->fY;
					else
					{
						DataPoint* pdpCurrent = pdpFirst;
						while(true)
						{
							if((pdpCurrent->fX < fX) && (fX <= static_cast<DataPoint*>(pdpCurrent->pNext)->fX))
							{
								return Map(fX, pdpCurrent->fX, pdpCurrent->fY, static_cast<DataPoint*>(pdpCurrent->pNext)->fX, static_cast<DataPoint*>(pdpCurrent->pNext)->fY);
							}
							else
							{
								pdpCurrent = static_cast<DataPoint*>(pdpCurrent->pNext);
							}
						}
					}
				}
			}
			return 0;
		}

	private:

		//Цепочечное удаление односвязного списка
		void CleanUp(SLList* pElement)
		{
			if(pElement)
			{
				SLList* pCurrent = pElement;
				SLList* pPrev = NULL;
				while(true)
				{
					//Запоминаем текущий, так как скоро он станет предыдущим
					pPrev = pCurrent;
					if(pCurrent->pNext)
					{
						//Есть ещё
						pCurrent = static_cast<SLList*>(pCurrent->pNext);
					}
					else
					{
						//Последний и есть текущий
						if(DebugPrint) fprintf(DebugPrint, "CleanUp: deleting element at %x\n", pCurrent);
						delete pCurrent;
						break;
					}
					if(DebugPrint) fprintf(DebugPrint, "CleanUp: deleting element at %x\n", pPrev);
					delete pPrev;
				}
			}
			else
				if(DebugPrint) fprintf(DebugPrint, "CleanUp: nothing to delete\n");
		}

		//Линейная интерполяция/экстраполяция
		float Map(const float& fQx, const float& fLX, const float& fLY, const float& fRX, const float& fRY) const
		{
			return ( (fQx - fLX) / (fRX - fLX) ) * (fRY - fLY) + fLY;
		}

		void AddMetric(const float& fLowerCandidateEdge, const float& fHigherCandidateEdge, const float& fDataToProjectionRatio, const float& fLowerProjectionEdge, const float& fHigherProjectionEdge)
		{
			//Создаём экземпляр структуры Metrics и заполняем его поля по умолчанию
			Metrics* pmNew = new Metrics();
			pmNew->fLowerCandidateEdge = fLowerCandidateEdge;
			pmNew->fMiddleCandidateEdge = fLowerCandidateEdge + fDataToProjectionRatio * (fHigherCandidateEdge - fLowerCandidateEdge);
			pmNew->fHigherCandidateEdge = fHigherCandidateEdge;
			pmNew->fLowerProjectionEdge = fLowerProjectionEdge;
			pmNew->fHigherProjectionEdge = fHigherProjectionEdge;
			pmNew->fWeight = 0;
			pmNew->fProjectionMean = 0;
			pmNew->fScale = 0;
			pmNew->fCandidateMean = 0;
			pmNew->pNext = NULL;

			//Должны сопоставлять коридор кандидата [fLowerCandidateEdge, fMiddleCandidateEdge]
			//	коридору проекции [fLowerProjectionEdge, fHigherProjectionEdge]
			long lStep;

			//Сама метрика (не сам pmNew->fWeight, а то, из чего он будет получен)
			float fMetrics;

			//Считаем размер шагов в соответствующих окнах
			float fCandidateStepSize = (pmNew->fMiddleCandidateEdge - fLowerCandidateEdge) / (float)(lMetricStepsCount);
			float fProjectionStepSize = (fHigherProjectionEdge - fLowerProjectionEdge) / (float)(lMetricStepsCount);

			//Временные массивы для значений окон кандидата и проекции
			//	(а то слишком много вызовов GetData)
			float* pfCandidateY = new float[lMetricStepsCount + 1];
			float* pfProjectionY = new float[lMetricStepsCount + 1];

			//Заполним временные массивы
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				pfCandidateY[lStep] = GetData(fLowerCandidateEdge + fCandidateStepSize * (float)(lStep));
				pfProjectionY[lStep] = GetData(fLowerProjectionEdge + fProjectionStepSize * (float)(lStep));
			}

			//Вначале тут сумма, потом среднее арифметическое
			float fCandidateMean = 0.0;
			float fProjectionMean = 0.0;
			float fCandidateDisp = 0.0;
			float fProjectionDisp = 0.0;

			//Считаем матожидание в окнах
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				fCandidateMean += pfCandidateY[lStep];
				fProjectionMean += pfProjectionY[lStep];
			}
			fCandidateMean /= (float)(lMetricStepsCount + 1);
			fProjectionMean /= (float)(lMetricStepsCount + 1);

			pmNew->fCandidateMean = fCandidateMean;
			pmNew->fProjectionMean = fProjectionMean;

			//Считаем дисперсию в окнах
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				fCandidateDisp += (pfCandidateY[lStep] - fCandidateMean) * (pfCandidateY[lStep] - fCandidateMean);
				fProjectionDisp += (pfProjectionY[lStep] - fProjectionMean) * (pfProjectionY[lStep] - fProjectionMean);
			}
			fCandidateDisp = sqrt(fCandidateDisp / (float)(lMetricStepsCount + 1));
			fProjectionDisp = sqrt(fProjectionDisp / (float)(lMetricStepsCount + 1));

			//Получили масштаб
			if(fCandidateDisp > 0)
				pmNew->fScale = fProjectionDisp / fCandidateDisp;
			else
				pmNew->fScale = 1.0;

			//Считаем метрику
			fMetrics = 0.0;
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				//Накапливаем погрешность между двумя точками данных
				//	чем больше fMetrics, тем более непохожи данные
				//	чем меньше fMetrics, тем более похожи данные
				fMetrics += fabs(pfProjectionY[lStep] - ((pfCandidateY[lStep] - fCandidateMean) * pmNew->fScale + pmNew->fProjectionMean));
			}

			fMetrics /= (float)(lMetricStepsCount + 1);

			//Получили весовой коэффициент
			pmNew->fWeight = exp(-fMetrics * fMetrics);	//для нивелирования влияния несовпавших кандидатов необходимо повышать степень

			fprintf(DebugPrint, "AddMetric: Candidate = [%f, %f, %f], Projection = [%f, %f], Candidate = {%f, %f}, Projection = {%f, %f}, Scale = %f, Difference = %f, Weight = %f\n", fLowerCandidateEdge, pmNew->fMiddleCandidateEdge, fHigherCandidateEdge, fLowerProjectionEdge, fHigherProjectionEdge, fCandidateMean, fCandidateDisp, fProjectionMean, fProjectionDisp, pmNew->fScale, fMetrics, pmNew->fWeight);

			//Удаляем временные массивы
			delete [] pfCandidateY;
			delete [] pfProjectionY;

			//В конце добавим вычисленное в список
			if(pmFirst)
			{
				pmLast->pNext = pmNew;
				pmLast = pmNew;
			}
			else
			{
				pmFirst = pmNew;
				pmLast = pmNew;
			}
		}

		//Производит прогнозирование
		void Forecast(const float& fUpperProjectionEdge, const float& fStepSize, const long& uStepsCount)
		{
			if(bIsForecasted)
				return;

			//Запомним минимальное и максимальное X
			float fXMin = pdpFirst->fX;
			float fXMax = pdpLast->fX;

			//Определяем отступ для минимальных размеров окон
			float fMargin = fUpperProjectionEdge - fXMax;

			//Границы и размер шагов окна проекции
			float fLowerProjectionEdge;
			float fLowerProjectionStepSize = (fXMax - fMargin - fXMin) / (float)(lProjectionStepsCount);
			float fDataToProjectionRatio;

			//Границы и размер шагов окна кандидатов
			float fLowerCandidateEdge;
			float fCandidateStepSize = (fXMax - fMargin - fXMin) / (float)(lCandidateStepsCount);
			float fHigherCandidateEdge;

			//Шагающие
			long lLowerProjectionStep = 0;
			long lLowerCandidateStep = 0;
			long lHigherCandidateStep = 0;

			long lHigherCandidateCorrection = 1 + (fMargin / fCandidateStepSize);

			if(DebugPrint)
			{
				fprintf(DebugPrint, "Forecast: Upper projection edge = %f, Step size = %f, Steps count = %i\n", fUpperProjectionEdge, fStepSize, uStepsCount);
				fprintf(DebugPrint, "Forecast: Domain = [%f, %f], Margin = %f\n", fXMin, fXMax, fMargin);
				fprintf(DebugPrint, "Forecast: Candidate step size = %f, Projection step size = %f\n", fCandidateStepSize, fLowerProjectionStepSize);
			}

			//Рассчитываем
			for(lLowerProjectionStep = 0; lLowerProjectionStep < lProjectionStepsCount; lLowerProjectionStep++)
			{
				fLowerProjectionEdge = fXMin + fLowerProjectionStepSize * (float)(lLowerProjectionStep);
				fDataToProjectionRatio = (fXMax - fLowerProjectionEdge) / (fUpperProjectionEdge - fLowerProjectionEdge);
				for(lLowerCandidateStep = 0; lLowerCandidateStep < lCandidateStepsCount; lLowerCandidateStep++)
				{
					fLowerCandidateEdge = fXMin + fCandidateStepSize * (float)(lLowerCandidateStep);
					for(lHigherCandidateStep = lLowerCandidateStep + lHigherCandidateCorrection; lHigherCandidateStep < lCandidateStepsCount + lHigherCandidateCorrection; lHigherCandidateStep++)
					{
						fHigherCandidateEdge = fXMin + fCandidateStepSize * (float)(lHigherCandidateStep);

						AddMetric(	fLowerCandidateEdge,
									fHigherCandidateEdge,
									fDataToProjectionRatio,
									fLowerProjectionEdge,
									fXMax	);

					}	//for @ fHigherCandidateEdge

				}	//for @ fLowerCandidateEdge

			}	//for @ fLowerProjectionEdge

			//Reuse
			Metrics* pmCurrent;

			//Нормируем весовые коэффициенты
			if(DebugPrint) fprintf(DebugPrint, "Forecast: Weights: ");
			if(pmFirst)
			{
				float fSum = 0.0;

				pmCurrent = pmFirst;
				while(pmCurrent)
				{
					fSum += pmCurrent->fWeight;
					pmCurrent = static_cast<Metrics*>(pmCurrent->pNext);
				}

				if(fSum > 0.0)
				{
					pmCurrent = pmFirst;
					while(pmCurrent)
					{
						pmCurrent->fWeight /= fSum;
						if(DebugPrint && (pmCurrent->fWeight > 0.0)) fprintf(DebugPrint, "%f ", pmCurrent->fWeight);
						pmCurrent = static_cast<Metrics*>(pmCurrent->pNext);
					}
				}
			}
			if(DebugPrint) fprintf(DebugPrint, "\n");

			//Вычисление финального результата
			if(pmFirst)
			{
				float fCashePointX;
				float fCashePointY;
				//Проходим по всем точкам, значения в которых надо закешировать
				for(fCashePointX = fUpperProjectionEdge; fXMax < fCashePointX; fCashePointX -= fStepSize)
				{
					fCashePointY = 0.0;

					//Проходим по всем метрикам
					pmCurrent = pmFirst;
					while(pmCurrent)
					{
						//Считаем пропорцию точки
						float fSamplePointXRatio = (fCashePointX - pmCurrent->fLowerProjectionEdge) / (fUpperProjectionEdge - pmCurrent->fLowerProjectionEdge);

						//Находим соответствующую точку в окне кандидатов
						float fCandidateSamplePointX = pmCurrent->fLowerCandidateEdge + fSamplePointXRatio * (pmCurrent->fHigherCandidateEdge - pmCurrent->fLowerCandidateEdge);

						//Аккумулируем взвешенное смещённое значение
						fCashePointY += pmCurrent->fWeight * ((GetData(fCandidateSamplePointX) - pmCurrent->fCandidateMean) * pmCurrent->fScale + pmCurrent->fProjectionMean);

						//
						pmCurrent = static_cast<Metrics*>(pmCurrent->pNext);
					}

					//Добавим точку (fCashePointX, fCashePointY) к данным как подсчитанную
					AddPoint(fCashePointX, fCashePointY, false);
				}
			}

			bIsForecasted = true;
		}

		//Оригинальное добавление
		bool AddPoint(const float& fX, const float& fY, const char& bIsOriginal)
		{
			if(DebugPrint) fprintf(DebugPrint, "AddPoint: (%f, %f) ", fX, fY);

			if(bIsForecasted)
				return false;

			bool bIsResult = false;

			//Уже есть данные?
			if(pdpFirst)
			{
				//Создаём новую запись
				DataPoint* pdpNew = new DataPoint(fX, fY, bIsOriginal);

				//Неявная оптимизация
				if(pdpLast)
				{
					if(fX > pdpLast->fX)
					{
						if(DebugPrint) fprintf(DebugPrint, "after last (%f, %f)", pdpLast->fX, pdpLast->fY);

						bIsResult = true;
						//Залинковываем с самой крайней правой
						pdpLast->pNext = pdpNew;
						//Так как она самая правая, то последняя теперь она
						pdpLast = pdpNew;
					}
				}

				DataPoint* pdpCurrent = pdpFirst;

				while(!bIsResult)
				{
					//Сравниваем с текущим (проверка на коллизию)
					if(fX == pdpCurrent->fX)
					{
						break;
					}
					else
					{
						if(fX < pdpCurrent->fX)
						{
							if(DebugPrint) fprintf(DebugPrint, "before first (%f, %f)", pdpFirst->fX, pdpFirst->fY);

							bIsResult = true;
							//Залинковываем с самой крайней левой
							pdpNew->pNext = pdpCurrent;
							if(pdpFirst == pdpCurrent)
							{
								//Так как она самая левая, то первая теперь она
								pdpFirst = pdpNew;
							}
							break;
						}
						else
						{
							//Есть ли кто-то спереди
							if(pdpCurrent->pNext)
							{
								//Спереди кто-то есть
								if(fX < static_cast<DataPoint*>(pdpCurrent->pNext)->fX)
								{
									if(DebugPrint) fprintf(DebugPrint, "between (%f, %f) and (%f, %f)", pdpCurrent->fX, pdpCurrent->fY, static_cast<DataPoint*>(pdpCurrent->pNext)->fX, static_cast<DataPoint*>(pdpCurrent->pNext)->fY);

									//Вставка между двумя точками данных
									bIsResult = true;
									//Залинковываем с правым соседом
									pdpNew->pNext = pdpCurrent->pNext;
									//Двухсторонне залинковываем с левым соседом
									pdpCurrent->pNext = pdpNew;
									//Последний и первый не меняются
									break;
								}
								else
								{
									//Попали в ситуацию, когда мы до младшего
									pdpCurrent = static_cast<DataPoint*>(pdpCurrent->pNext);
									//Повтор...
								}
							}
						}
					}
				}

			}
			else
			{
				//Первые данные
				pdpFirst = new DataPoint(fX, fY, bIsOriginal);
				bIsResult = true;
				pdpLast = pdpFirst;

				if(DebugPrint) fprintf(DebugPrint, "as first");
			}

			if(DebugPrint) fprintf(DebugPrint, "\n");

			return bIsResult;
		}
	};
}

#endif