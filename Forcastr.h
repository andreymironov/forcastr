#ifndef FORCASTR
#define FORCASTR

#define DP_ORIGINAL		0
#define DP_CALCULATED	1

#include <math.h>
#include <stdio.h>

namespace Forcastr
{
	#define NULL	0

	//����������� ������
	struct SLList
	{
		void* pNext;

		SLList()
		{
			pNext = NULL;
		}
	};

	//��������� �������� �������� � ������������� ������
	struct DataPoint:SLList
	{
		//��������
		float fX;
		//��������
		float fY;
		//������������ �� ������?
		char bIsOriginal;

		DataPoint(float fX, float fY, char bIsOriginal) : fX(fX), fY(fY), bIsOriginal(bIsOriginal)
		{
			bIsOriginal = DP_ORIGINAL;
		}
	};

	//�������� ������ � �������
	struct Metrics:SLList
	{
		//���� ���������
		float fLowerCandidateEdge;
		float fMiddleCandidateEdge;
		float fHigherCandidateEdge;
		//���� ��������
		float fLowerProjectionEdge;
		float fHigherProjectionEdge;
		//������� �����������
		float fWeight;
		//�������
		float fScale;
		//����������� �� ���������
		float fCandidateMean;
		//����������� �� ��������
		float fProjectionMean;
	};

	class Forcastr
	{
	public:

	private:
		bool bIsForecasted;				//���� �� ��� ������� ������� ������������
		DataPoint* pdpFirst;			//������ ������� ������
		DataPoint* pdpLast;				//��������� ������� ������
		long lProjectionStepsCount;		//��� ���� ��������
		long lCandidateStepsCount;		//��� ���� ���������
		long lMetricStepsCount;			//��� �������� �������
		Metrics* pmFirst;				//������ ������� ������ ������
		Metrics* pmLast;				//��������� ������� ������ ������
		FILE* DebugPrint;				//��������� �� ���� ��� ������

	public:

		//����������
		~Forcastr(void)
		{
			//�������� ������������ ������ DataPoint
			CleanUp(pdpFirst);
			//�������� ������������ ������ ������
			CleanUp(pmFirst);
		}

		void Initialize(void)
		{
			//�� ��������� �� ����������
			bIsForecasted = false;

			//��� ������
			pdpFirst = NULL;
			pdpLast = NULL;

			//��� ������
			pmFirst = NULL;
			pmLast = NULL;

			DebugPrint = NULL;

			//��������� ���������� �����
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

		//��������� ����� � �������� ������
		bool AddPoint(const float& fX, const float& fY)
		{
			return AddPoint(fX, fY, DP_ORIGINAL);
		}

		//���������� ���������������
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

		//���������� �������� Y ��� ���������� X �� �������� ������
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

		//���������� �������� ������������ ������
		void CleanUp(SLList* pElement)
		{
			if(pElement)
			{
				SLList* pCurrent = pElement;
				SLList* pPrev = NULL;
				while(true)
				{
					//���������� �������, ��� ��� ����� �� ������ ����������
					pPrev = pCurrent;
					if(pCurrent->pNext)
					{
						//���� ���
						pCurrent = static_cast<SLList*>(pCurrent->pNext);
					}
					else
					{
						//��������� � ���� �������
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

		//�������� ������������/�������������
		float Map(const float& fQx, const float& fLX, const float& fLY, const float& fRX, const float& fRY) const
		{
			return ( (fQx - fLX) / (fRX - fLX) ) * (fRY - fLY) + fLY;
		}

		void AddMetric(const float& fLowerCandidateEdge, const float& fHigherCandidateEdge, const float& fDataToProjectionRatio, const float& fLowerProjectionEdge, const float& fHigherProjectionEdge)
		{
			//������ ��������� ��������� Metrics � ��������� ��� ���� �� ���������
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

			//������ ������������ ������� ��������� [fLowerCandidateEdge, fMiddleCandidateEdge]
			//	�������� �������� [fLowerProjectionEdge, fHigherProjectionEdge]
			long lStep;

			//���� ������� (�� ��� pmNew->fWeight, � ��, �� ���� �� ����� �������)
			float fMetrics;

			//������� ������ ����� � ��������������� �����
			float fCandidateStepSize = (pmNew->fMiddleCandidateEdge - fLowerCandidateEdge) / (float)(lMetricStepsCount);
			float fProjectionStepSize = (fHigherProjectionEdge - fLowerProjectionEdge) / (float)(lMetricStepsCount);

			//��������� ������� ��� �������� ���� ��������� � ��������
			//	(� �� ������� ����� ������� GetData)
			float* pfCandidateY = new float[lMetricStepsCount + 1];
			float* pfProjectionY = new float[lMetricStepsCount + 1];

			//�������� ��������� �������
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				pfCandidateY[lStep] = GetData(fLowerCandidateEdge + fCandidateStepSize * (float)(lStep));
				pfProjectionY[lStep] = GetData(fLowerProjectionEdge + fProjectionStepSize * (float)(lStep));
			}

			//������� ��� �����, ����� ������� ��������������
			float fCandidateMean = 0.0;
			float fProjectionMean = 0.0;
			float fCandidateDisp = 0.0;
			float fProjectionDisp = 0.0;

			//������� ����������� � �����
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				fCandidateMean += pfCandidateY[lStep];
				fProjectionMean += pfProjectionY[lStep];
			}
			fCandidateMean /= (float)(lMetricStepsCount + 1);
			fProjectionMean /= (float)(lMetricStepsCount + 1);

			pmNew->fCandidateMean = fCandidateMean;
			pmNew->fProjectionMean = fProjectionMean;

			//������� ��������� � �����
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				fCandidateDisp += (pfCandidateY[lStep] - fCandidateMean) * (pfCandidateY[lStep] - fCandidateMean);
				fProjectionDisp += (pfProjectionY[lStep] - fProjectionMean) * (pfProjectionY[lStep] - fProjectionMean);
			}
			fCandidateDisp = sqrt(fCandidateDisp / (float)(lMetricStepsCount + 1));
			fProjectionDisp = sqrt(fProjectionDisp / (float)(lMetricStepsCount + 1));

			//�������� �������
			if(fCandidateDisp > 0)
				pmNew->fScale = fProjectionDisp / fCandidateDisp;
			else
				pmNew->fScale = 1.0;

			//������� �������
			fMetrics = 0.0;
			for(lStep = 0; lStep <= lMetricStepsCount; lStep++)
			{
				//����������� ����������� ����� ����� ������� ������
				//	��� ������ fMetrics, ��� ����� �������� ������
				//	��� ������ fMetrics, ��� ����� ������ ������
				fMetrics += fabs(pfProjectionY[lStep] - ((pfCandidateY[lStep] - fCandidateMean) * pmNew->fScale + pmNew->fProjectionMean));
			}

			fMetrics /= (float)(lMetricStepsCount + 1);

			//�������� ������� �����������
			pmNew->fWeight = exp(-fMetrics * fMetrics);	//��� ������������� ������� ����������� ���������� ���������� �������� �������

			fprintf(DebugPrint, "AddMetric: Candidate = [%f, %f, %f], Projection = [%f, %f], Candidate = {%f, %f}, Projection = {%f, %f}, Scale = %f, Difference = %f, Weight = %f\n", fLowerCandidateEdge, pmNew->fMiddleCandidateEdge, fHigherCandidateEdge, fLowerProjectionEdge, fHigherProjectionEdge, fCandidateMean, fCandidateDisp, fProjectionMean, fProjectionDisp, pmNew->fScale, fMetrics, pmNew->fWeight);

			//������� ��������� �������
			delete [] pfCandidateY;
			delete [] pfProjectionY;

			//� ����� ������� ����������� � ������
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

		//���������� ���������������
		void Forecast(const float& fUpperProjectionEdge, const float& fStepSize, const long& uStepsCount)
		{
			if(bIsForecasted)
				return;

			//�������� ����������� � ������������ X
			float fXMin = pdpFirst->fX;
			float fXMax = pdpLast->fX;

			//���������� ������ ��� ����������� �������� ����
			float fMargin = fUpperProjectionEdge - fXMax;

			//������� � ������ ����� ���� ��������
			float fLowerProjectionEdge;
			float fLowerProjectionStepSize = (fXMax - fMargin - fXMin) / (float)(lProjectionStepsCount);
			float fDataToProjectionRatio;

			//������� � ������ ����� ���� ����������
			float fLowerCandidateEdge;
			float fCandidateStepSize = (fXMax - fMargin - fXMin) / (float)(lCandidateStepsCount);
			float fHigherCandidateEdge;

			//��������
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

			//������������
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

			//��������� ������� ������������
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

			//���������� ���������� ����������
			if(pmFirst)
			{
				float fCashePointX;
				float fCashePointY;
				//�������� �� ���� ������, �������� � ������� ���� ������������
				for(fCashePointX = fUpperProjectionEdge; fXMax < fCashePointX; fCashePointX -= fStepSize)
				{
					fCashePointY = 0.0;

					//�������� �� ���� ��������
					pmCurrent = pmFirst;
					while(pmCurrent)
					{
						//������� ��������� �����
						float fSamplePointXRatio = (fCashePointX - pmCurrent->fLowerProjectionEdge) / (fUpperProjectionEdge - pmCurrent->fLowerProjectionEdge);

						//������� ��������������� ����� � ���� ����������
						float fCandidateSamplePointX = pmCurrent->fLowerCandidateEdge + fSamplePointXRatio * (pmCurrent->fHigherCandidateEdge - pmCurrent->fLowerCandidateEdge);

						//������������ ���������� ��������� ��������
						fCashePointY += pmCurrent->fWeight * ((GetData(fCandidateSamplePointX) - pmCurrent->fCandidateMean) * pmCurrent->fScale + pmCurrent->fProjectionMean);

						//
						pmCurrent = static_cast<Metrics*>(pmCurrent->pNext);
					}

					//������� ����� (fCashePointX, fCashePointY) � ������ ��� ������������
					AddPoint(fCashePointX, fCashePointY, false);
				}
			}

			bIsForecasted = true;
		}

		//������������ ����������
		bool AddPoint(const float& fX, const float& fY, const char& bIsOriginal)
		{
			if(DebugPrint) fprintf(DebugPrint, "AddPoint: (%f, %f) ", fX, fY);

			if(bIsForecasted)
				return false;

			bool bIsResult = false;

			//��� ���� ������?
			if(pdpFirst)
			{
				//������ ����� ������
				DataPoint* pdpNew = new DataPoint(fX, fY, bIsOriginal);

				//������� �����������
				if(pdpLast)
				{
					if(fX > pdpLast->fX)
					{
						if(DebugPrint) fprintf(DebugPrint, "after last (%f, %f)", pdpLast->fX, pdpLast->fY);

						bIsResult = true;
						//������������� � ����� ������� ������
						pdpLast->pNext = pdpNew;
						//��� ��� ��� ����� ������, �� ��������� ������ ���
						pdpLast = pdpNew;
					}
				}

				DataPoint* pdpCurrent = pdpFirst;

				while(!bIsResult)
				{
					//���������� � ������� (�������� �� ��������)
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
							//������������� � ����� ������� �����
							pdpNew->pNext = pdpCurrent;
							if(pdpFirst == pdpCurrent)
							{
								//��� ��� ��� ����� �����, �� ������ ������ ���
								pdpFirst = pdpNew;
							}
							break;
						}
						else
						{
							//���� �� ���-�� �������
							if(pdpCurrent->pNext)
							{
								//������� ���-�� ����
								if(fX < static_cast<DataPoint*>(pdpCurrent->pNext)->fX)
								{
									if(DebugPrint) fprintf(DebugPrint, "between (%f, %f) and (%f, %f)", pdpCurrent->fX, pdpCurrent->fY, static_cast<DataPoint*>(pdpCurrent->pNext)->fX, static_cast<DataPoint*>(pdpCurrent->pNext)->fY);

									//������� ����� ����� ������� ������
									bIsResult = true;
									//������������� � ������ �������
									pdpNew->pNext = pdpCurrent->pNext;
									//������������ ������������� � ����� �������
									pdpCurrent->pNext = pdpNew;
									//��������� � ������ �� ��������
									break;
								}
								else
								{
									//������ � ��������, ����� �� �� ��������
									pdpCurrent = static_cast<DataPoint*>(pdpCurrent->pNext);
									//������...
								}
							}
						}
					}
				}

			}
			else
			{
				//������ ������
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