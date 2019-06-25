﻿
#include <sis_core.h>
#include <sis_ai.h>

void gauss_solve(int n, double A[], double x[], double b[])
{
	int i, j, k, r;
	double max;
	for (k = 0; k < n - 1; k++)
	{
		max = fabs(A[k*n + k]); /*find maxmum*/
		r = k;
		for (i = k + 1; i < n - 1; i++){
			if (max < fabs(A[i*n + i]))
			{
				max = fabs(A[i*n + i]);
				r = i;
			}
		}
		if (r != k){
			for (i = 0; i < n; i++) /*change array:A[k]&A[r] */
			{
				max = A[k*n + i];
				A[k*n + i] = A[r*n + i];
				A[r*n + i] = max;
			}
		}
		max = b[k]; /*change array:b[k]&b[r] */
		b[k] = b[r];
		b[r] = max;
		for (i = k + 1; i < n; i++)
		{
			for (j = k + 1; j < n; j++)
			{
				A[i*n + j] -= A[i*n + k] * A[k*n + j] / A[k*n + k];
			}
			b[i] -= A[i*n + k] * b[k] / A[k*n + k];
		}
	}
	for (i = n - 1; i >= 0; x[i] /= A[i*n + i], i--)
	{
		for (j = i + 1, x[i] = b[i]; j < n; j++)
		{
			x[i] -= A[i*n + j] * x[j];
		}
	}

}
/*==================polyfit(n,x,y,poly_n,a)===================*/
/*=======拟合y=a0+a1*x+a2*x^2+……+apoly_n*x^poly_n========*/
/*=====n是数据个数 xy是数据值 poly_n是多项式的项数======*/
/*===返回a0,a1,a2,……a[poly_n]，系数比项数多一（常数项）=====*/
void polyfit(int n, double x[], double y[], int poly_n, double a[])
{
	int i, j;
	double *tempx, *tempy, *sumxx, *sumxy, *ata;
	//void gauss_solve(int n, double A[], double x[], double b[]);
	tempx = (double*)sis_calloc(n * sizeof(double));
	sumxx = (double*)sis_calloc((poly_n * 2 + 1) * sizeof(double));
	tempy = (double*)sis_calloc(n * sizeof(double));
	sumxy = (double*)sis_calloc((poly_n + 1) * sizeof(double));
	ata = (double*)sis_calloc(((poly_n + 1)*(poly_n + 1)) * sizeof(double));
	for (i = 0; i<n; i++)
	{
		tempx[i] = 1;
		tempy[i] = y[i];
	}
	for (i = 0; i < 2 * poly_n + 1; i++){
		for (sumxx[i] = 0, j = 0; j < n; j++)
		{
			sumxx[i] += tempx[j];
			tempx[j] *= x[j];
		}
	}
	for (i = 0; i < poly_n + 1; i++)
	{
		for (sumxy[i] = 0, j = 0; j < n; j++)
		{
			sumxy[i] += tempy[j];
			tempy[j] *= x[j];
		}
	}
	for (i = 0; i < poly_n + 1; i++)
	{
		for (j = 0; j < poly_n + 1; j++)
		{
			ata[i*(poly_n + 1) + j] = sumxx[i + j];
		}
	}
	gauss_solve(poly_n + 1, ata, a, sumxy);
	sis_free(tempx);
	sis_free(sumxx);
	sis_free(tempy);
	sis_free(sumxy);
	sis_free(ata);
}
double sis_ai_slope(int n, double ins[])
{
    double a[3];
    double *x = (double *)sis_calloc(sizeof(double) * n);
    for (int m = 0; m < n; m++)
    {
        x[m] = m + 1;
    }
    polyfit(n, x, ins, 1, a);
    sis_free(x);
    // printf("%f %f %f\n", a[0], a[1], a[2]);
    return a[1];
}
// -1 ... +1 之间
double sis_ai_slope_rate(int n, double ins[])
{
    if (n < 3) 
    {
        return  0.0;
    }
    double min = 0.0;
    double max = 0.0;

    double a[3];
    double *x = (double *)sis_calloc(sizeof(double) * n);
    for (int m = 0; m < n; m++)
    {
        x[m] = m;
        min = sis_min(min, ins[m]);
        max = sis_max(max, ins[m]);
        // printf("%f %f %f \n", ins[m], min , max);
    }
    double *y = (double *)sis_calloc(sizeof(double) * n);
    // 保证从 0 出发
    // printf("%f %f \n", min , max);
    for (int m = 0; m < n; m++)
    {
        // y[m] = (ins[m] - ins[0]) / max * m;
        y[m] = (ins[m] - min) / ((max - min) / (n - 1));
        // printf("%f %f %f\n", ins[m], x[m], y[m]);
    }    
    polyfit(n, x, y, 1, a);
    sis_free(x);
    sis_free(y);
    // printf("%f %f %f\n", a[0], a[1], a[2]);
    return a[1];
}

void   sis_ai_series_argv(int n, double ins[], double *avg, double *vari)
{
    *avg = 0.0;
    *vari = 0.0;
    if (n < 1)
    {
        return ;
    }
    double sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        sum += ins[i];
    }
    *avg = sum / (double)n;
    sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        sum += pow(ins[i] - *avg, 2);
    }    
    *vari = sqrt(sum / (n-1));
}
double sis_ai_series(double in, double avg, double vari)
{
    if (vari==0)
    {
        return 0.0;
    }
    return exp(-1*pow(in - avg, 2)/(2*pow(vari,2))) / (sqrt(2*SIS_AI_CONST_PAI)*vari);
}

/////////////////////////////////////////////////////
//  下面是通用的求组合结果的类
////////////////////////////////////////////////////

// 求速度 
s_sis_calc_cycle *sis_calc_cycle_create(int style_)
{
    s_sis_calc_cycle *calc = (s_sis_calc_cycle *)sis_malloc(sizeof(s_sis_calc_cycle));
    calc->style = style_;
    calc->list = sis_struct_list_create(sizeof(double), NULL, 0);
    return calc;
}
void sis_calc_cycle_destroy(s_sis_calc_cycle *calc_)
{
    sis_struct_list_destroy(calc_->list);
    sis_free(calc_);
}
int sis_calc_cycle_push(s_sis_calc_cycle *calc_, double value_)
{
    sis_struct_list_push(calc_->list, &value_);
    return calc_->list->count;
}
void sis_calc_cycle_clear(s_sis_calc_cycle *calc_)
{
    sis_struct_list_clear(calc_->list);
}

double *sis_calc_cycle_get(s_sis_calc_cycle *calc_)
{
    return (double *)sis_struct_list_get(calc_->list, 0);
}

int sis_calc_cycle_set_float(s_sis_calc_cycle *calc_, float *value_, int count_)
{  
    sis_struct_list_clear(calc_->list);
    if(calc_->style == JUDGE_STYLE_TREND)
    {
        int o = TREND_DIR_NONE;
        if (count_ < 2)
        {
            return o;
        }
        for(int i = 0; i < count_; i++)
        {
            double val = (double)value_[i];
            sis_struct_list_push(calc_->list, &val);
        }
        return sis_calc_cycle_out(calc_);
    }
    else
    {
        int o = SPEED_NONE_LESS;
        if (count_ < 3)
        {
            return o;
        }
        for(int i = 0; i < count_; i++)
        {
            double val = (double)value_[i];
            sis_struct_list_push(calc_->list, &val);
        }
        return sis_calc_cycle_out(calc_);
    }
    
}
///////////////////////////////////////////////////
// 计算速度时，碰到符号不同即停止计算
// 用第二个减去第一个，生成新的数列，加起来求均值，如果第一个差值大于均值表示速度越来越快，否则慢
// 求速度最少3个值，少于3个无法判断，通常利用连续的周期数据
//////////////////////////
// 求趋势，最少2个数值
// 第一个值大于第二个值，依次类推，如果都是这样，表示趋势形成
// 通常用菲数列周期来输入数据
/////////////////////////
int sis_calc_cycle_out(s_sis_calc_cycle *calc_)
{
    int o = JUDGE_STYLE_NONE;
    switch (calc_->style)
    {
        case JUDGE_STYLE_TREND:
            {
                o = TREND_DIR_NONE;
                int ups = 1, dns = 1;
                double ago = *(double *)sis_struct_list_get(calc_->list, 0);
                for(int i = 1; i < calc_->list->count; i++)
                {      
                    double val = *(double *)sis_struct_list_get(calc_->list, i);              
                    if (ago > val)
                    {
                        ups++;
                    }
                    if (ago < val)
                    {
                        dns++;
                    }
                    if(ups >= calc_->list->count)
                    {
                        o = TREND_DIR_INCR;
                        break;
                    }
                    if(dns >= calc_->list->count)
                    {
                        o = TREND_DIR_DECR;
                        break;
                    }
                    ago = val;
                }
            }
            break;
        case JUDGE_STYLE_SPEED:
            {
                o = SPEED_NONE_LESS;
                bool isup = false, isdn = false;
                int cycle = 0;

                for(int i = 0; i < calc_->list->count; i++)
                {
                    double val = *(double *)sis_struct_list_get(calc_->list, i);
                    if (!isup&&!isdn)
                    {
                        if (val  > 0.01)
                        {
                            isup = true;
                        }
                        if (val  < -0.01)
                        {
                            isdn = true;
                        } 
                        cycle++;
                        continue;
                    } 
                    if (isup&&val  < 0.01)
                    {
                        break;
                    }
                    if (isdn&&val  > -0.01)
                    {
                        break;
                    }
                    cycle++;
                }
                // cycle 为实际周期
                if(cycle < 3) 
                {
                    break;
                }
                double avg = 0.0;
                double first = *(double *)sis_struct_list_get(calc_->list, 0);
                double ago = first;
                for(int i = 1; i < cycle; i++)
                {      
                    double val = *(double *)sis_struct_list_get(calc_->list, i); 
                    if(i==1)
                    {
                        first = val - ago;
                    }
                    avg += val - ago;
                    // printf(" [%d:%d]-- %.2f, += %.2f - %.2f\n",cycle, i, avg, val, ago);
                    ago = val;
                }
                avg = avg / (cycle - 1);
                if (isup)
                {
                    // printf(" == %.2f,%.2f\n",avg, first);
                    if (avg > first)
                    {
                        o = SPEED_INCR_SLOW;
                    }
                    else 
                    {
                        o = SPEED_INCR_FAST;
                    }
                }
                if (isdn)
                {
                    if (avg < first)
                    {
                        o = SPEED_DECR_SLOW;
                    }
                    else 
                    {
                        o = SPEED_DECR_FAST;
                    }
                }
            }
            break;
        default:
            break;
    }
    
    return o;
}

#if 0
int main()
{
    int count = 7;
    // double in[7] = { 5.0,6.0,8.0,12.0,20.0};
    // double in[7] = { 5.0,10.0,14.0,12.0,11.0}; //越涨越快

    // double in[7] = { -5.0,-6.0,-8.0,-12.0,-20.0};
    float in[7] = { -5.0,-10.0,-14.0,-12.0,-11.0}; //越跌越快

    s_sis_calc_cycle *cycle = sis_calc_cycle_create(JUDGE_STYLE_SPEED);
    
    sis_calc_cycle_set_float(cycle,in, count);
    // for(int i = 0; i < count; i++)
    // {
    //     sis_calc_cycle_push(cycle, in[i]);
    // }
    int out = sis_calc_cycle_out(cycle);
    printf("out: %d\n", out);
    sis_calc_cycle_destroy(cycle);
 
    return 0;
}

#endif
#if 0
int main()
{
    // double in1[8] = { 1, 2, 3, 4, 5, 6, 7, 8};
    double in1[8] = { 2, 4, 6, 8, 10, 12, 14, 16};
    double in2[8] = { 3, 4, 5, 6, 7, 8, 9, 10};
    double in3[8] = { 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008};
    double in4[8] = { 1001, 1010, 1030, 1050, 1060, 1075, 1090, 1100};
    // double in3[8] = { 10, 20, 30, 40, 50, 60, 70, 80};
    // double in4[8] = { 30, 50, 70, 90, 110, 130, 150, 170};
    // double in3[8] = { 10, 9, 8, 7, 6, 4, 4, 3};
 
    // double in1[5] = { 312, 638, 338, 182, 1209};
    // double in2[5] = { 702, 1158, 1131, 27559, 1426};

    // double in3[5] = { 65727, 68336, 68383, 67642, 50425};
    // double in4[5] = { 50914, 48605, 50597, 51816, 60696};

    int count = 8;
    // for (int i = 0; i < count; i++)
    // {
    //     in4[i] = -1 *in4[i];
    // }
    printf("slope: %.5f\n", sis_ai_slope_rate(count, in1));
    printf("slope: %.5f\n", sis_ai_slope_rate(count, in2));
    printf("slope: %.5f\n", sis_ai_slope_rate(count, in3));
    printf("slope: %.5f\n", sis_ai_slope_rate(count, in4));

    return 0;
}
#endif
#if 0
int main()
{
    double good[8] = { 0.697, 0.774, 0.634, 0.608, 0.556, 0.403, 0.481, 0.437};
    double error[9] = { 0.666, 0.243, 0.245, 0.343, 0.639, 0.657, 0.360, 0.593, 0.719};
    
    double avg;
    double vari;
    double value;

    int count = 8;
    sis_ai_series_argv(count, good, &avg, &vari);
    printf("para: %.5f %.5f\n", avg, vari);
 
    value = sis_ai_series(good[0], avg, vari);
    printf("out: %.5f\n", value);

    count = 9;
    sis_ai_series_argv(count, error, &avg, &vari);
    printf("para: %.5f %.5f\n", avg, vari);
 
    value = sis_ai_series(good[0], avg, vari);
    printf("out: %.5f\n", value);

    s_sis_calc_cycle *list = sis_calc_cycle_create(0);    
    for (int i = 0; i < 8; i++)
    {
        count = sis_calc_cycle_push(list, good[i]*1000); 
    }
    sis_ai_series_argv(count, sis_calc_cycle_get(list), &avg, &vari);
    printf("para: %.5f %.5f %.5f\n", avg, vari, 
        sis_ai_series( 1697 ,avg, vari));

    sis_calc_cycle_destroy(list);
    return 0;
}

#endif