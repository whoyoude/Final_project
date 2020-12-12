#include "init.h"
#include <stdint.h>
#include <stdlib.h>
DAC_HandleTypeDef hDAC1;
DMA_HandleTypeDef dma1;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;
UART_HandleTypeDef uart_init1;
uint16_t x[200];
uint16_t y[200];
extern uint16_t lose[];//Sound effect triggered when hit the obstacle
extern uint16_t jump[];//Sound effect triggered when hit jump
extern uint16_t start[];//Sound effect trigger when game start
int update_screen_flag=0;
int update_game_counter=0;
float time=0;
int pass=0;
int button1 = 0;
int button2 =0;
int button1_delay=0;
int button2_delay=0;
char user_m[1];
uint16_t value;

struct game_def{
    int state;//0 for menu, 1 for ready to start, 2 for running, 3 for loss
    int speed;//0,1
    int jump;
    int blocks[3];
}game = {0,0,0,{80,110,130}};

void Init_timer_HAL();
void configureDAC();
void convert_magnitude();//Convert element in the array of sound effect to make a better quality
void play_music(uint8_t choice);//0 -> start ; 1-> lose; 2-> jump
void update_screen();
void update_game();
void print_menu();
void print_game();
void print_player();
void print_blocks();
void print_result();
void uart_init();

int main(void){
	Sys_Init();
	configureDAC();
	Init_timer_HAL();
	convert_magnitude();
	HAL_TIM_Base_Start(&htim6);
	uart_init();
	HAL_UART_Receive_IT(&uart_init1, (uint8_t *)user_m, 1);
	printf("\033[2J\033[;H\033[0;36;40m\033[?25l");
	fflush(stdout);

	while(1){
		fflush(stdout);
		if(update_game_counter>=game.speed+1){
			update_game();
			update_game_counter=0;
		}
		if(button1){
			switch(game.state){
			case 0:
				if(game.speed==0) game.speed=1;
				else if(game.speed==1) game.speed=2;
				else if(game.speed==2) game.speed=0;

				break;
			case 1:
				break;
			}
			button1=0;
		}
		if(button2){
			switch(game.state){
			case 0:
				time=0;
				pass=0;
				game.state = 1;
				play_music(0);
				break;
			case 1:
				if(game.jump==0){
					game.jump=1;
					play_music(2);
				}
				break;

			}
			button2=0;
		}
	}
}

void DMA1_Stream5_IRQHandler(){
	HAL_DMA_IRQHandler(&dma1);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac){
	HAL_DAC_Stop_DMA(&hDAC1, DAC_CHANNEL_1);
}

void convert_magnitude(){
	uint8_t v = 0;
	for (uint16_t i = 0;i <43886;i++){//Convert the magnitude of element in array of sound effect 'Lose'
		v = lose[i]/2;
		lose[i] = v;
	}
	for (uint16_t i = 0;i <22576;i++){//Convert the magnitude of element in array of sound effect 'Jump'
		v = jump[i]/2;
		jump[i] = v;
	}
	for (uint16_t i = 0;i <22151;i++){//Convert the magnitude of element in array of sound effect 'Start'
		v = start[i]/2;
		start[i] = v;
	}
}

void play_music(uint8_t choice){
	if(choice == 0){//Play sound effect 'Start'
		HAL_DAC_Start_DMA(&hDAC1, DAC_CHANNEL_1, (uint32_t*)start,22152, DAC_ALIGN_12B_R);
	}
	else if(choice == 1){//Play sound effect 'Jump'
		HAL_DAC_Start_DMA(&hDAC1, DAC_CHANNEL_1, (uint32_t*)lose,43886, DAC_ALIGN_12B_R);
	}
	else if(choice == 2){//Play sound effect 'Lose'
		HAL_DAC_Start_DMA(&hDAC1, DAC_CHANNEL_1, (uint32_t*)jump,22576, DAC_ALIGN_12B_R);
	}
}


void Init_timer_HAL(){
	TIM_MasterConfigTypeDef sMasterConfig;

	//Enable clock
	__HAL_RCC_TIM6_CLK_ENABLE();

	//Set the TIM to be TIM6 and the corresponding prescaler and period
	htim6.Instance = TIM6;
	htim6.Init.Prescaler =0;
	htim6.Init.Period = 2448;//The rate of overflow is 44100 Hz
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

	//Initiate the timer
	HAL_TIM_Base_Init(&htim6);
	
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig);


	HAL_Init();
	HAL_TIM_Base_MspInit(&htim7);

	//Set the TIM to be TIM6 and the corresponding prescaler and period
	htim7.Instance = TIM7;
	htim7.Init.Prescaler =2249;
	htim7.Init.Period = 999;

	//Enable clock
	__HAL_RCC_TIM7_CLK_ENABLE();

	//Set priority and enable IRQ
	HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM7_IRQn);

	//Initiate the timer
	HAL_TIM_Base_Init(&htim7);
	HAL_TIM_Base_Start_IT(&htim7);
}

void configureDAC(){
	__HAL_RCC_DAC_CLK_ENABLE();

	hDAC1.Instance = DAC;
	HAL_DAC_Init(&hDAC1);

	DAC_ChannelConfTypeDef dacchan;
	dacchan.DAC_Trigger = DAC_TRIGGER_T6_TRGO;//The DAC event triggered by TIM6
	dacchan.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
	HAL_DAC_ConfigChannel(&hDAC1, &dacchan, DAC_CHANNEL_1);
}

void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
    if(hdac->Instance == DAC){
    	// Enable GPIO Clocks
       	__HAL_RCC_GPIOA_CLK_ENABLE();

    	// Initialize Pin
    	GPIO_InitStruct.Pin       = GPIO_PIN_4;
        GPIO_InitStruct.Mode      = GPIO_MODE_ANALOG;
    	GPIO_InitStruct.Pull      = GPIO_NOPULL;
    	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    	//Set the DMA and connect to DAC
    	 __HAL_RCC_DMA1_CLK_ENABLE();
    	 dma1.Instance = DMA1_Stream5;
    	 dma1.Init.Channel = DMA_CHANNEL_7;
    	 dma1.Init.Direction = DMA_MEMORY_TO_PERIPH;
         dma1.Init.PeriphInc = DMA_PINC_DISABLE;
    	 dma1.Init.MemInc = DMA_MINC_ENABLE;
    	 dma1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    	 dma1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    	 dma1.Init.Mode = DMA_NORMAL;
    	 dma1.Init.Priority = DMA_PRIORITY_LOW;
    	 dma1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    	 HAL_DMA_Init(&dma1);

    	 __HAL_LINKDMA(hdac, DMA_Handle1,dma1);

    	 HAL_NVIC_SetPriority(DMA1_Stream5_IRQn,0,0);
    	 HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    }
  }

void TIM7_IRQHandler(){
	HAL_TIM_IRQHandler(&htim7);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM7){//Increment the time
		update_screen_flag++;
		update_game_counter++;
	}
	if(update_screen_flag>=3){
		update_screen();
		update_screen_flag=0;
	}
	if(button1_delay>0) button1_delay--;
	if(button2_delay>0) button2_delay--;
}

//PJ0=D4  PC8=D5
void uart_init(){
    HAL_NVIC_SetPriority(USART1_IRQn,0,1);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    __HAL_RCC_USART1_CLK_ENABLE();

    uart_init1.Instance = USART1;
    uart_init1.Init.BaudRate = 115200;
    uart_init1.Init.WordLength = UART_WORDLENGTH_8B;
    uart_init1.Init.StopBits = UART_STOPBITS_1;
    uart_init1.Init.Parity     = UART_PARITY_NONE;
    uart_init1.Init.Mode       = UART_MODE_TX_RX;
    uart_init1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	HAL_UART_Init(&uart_init1);
	//HAL_UART_MspInit(&uart_init1);
	//__HAL_UART_ENABLE_IT(&uart_init1, UART_IT_RXNE);


}


void USART1_IRQHandler(){
	HAL_UART_IRQHandler(&uart_init1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart_init){
	HAL_UART_Receive_IT(&uart_init1, (uint8_t *)user_m, 1);
	if(user_m[0] == '\033' && game.state==3){
		printf("\033[2J\033[10;38HEXIT");
		exit(0);
	}
	else if(user_m[0] != '\033' && game.state==3){
		game.state = 0;
		button2_delay=5;
	}
	else if(user_m[0] == 'w' && button1_delay==0){
		button1 = 1;
		button1_delay=5;
	}
	else if(user_m[0] == ' ' && button2_delay==0){
		button2 = 1;
		button2_delay=5;
	}


}


void update_screen(){
	switch(game.state){
	case 0:
		print_menu();
		break;
	case 1:
		print_game();
		break;
	case 2:
		print_game();
		break;
	case 3:
		print_result();
	}
	fflush(stdout);

}

void print_menu(){
	printf("\033[2J\033[;H\033[0;36;40m");
	printf("\033[1;1Hpass: %d",pass);
	printf("\033[2;1Htime: %ds",(int)(time/24));
	printf("\033[10;38HJUMP GAME\033[12;20Hpress space to start, press w to switch speed");
	switch(game.speed){
	case 0:
		printf("\033[14;40Hfast");
		break;
	case 1:
		printf("\033[14;40Hmedium");
		break;
	case 2:
		printf("\033[14;40Hslow");
		break;
	}
}

void print_game(){
	printf("\033[2J\033[;H\033[0;36;40m");
	printf("\033[20;1H");
	printf("-------------------------------------------------------------------------------");
	print_player();
	print_blocks();
	printf("\033[1;1Hpass: %d",pass);
	printf("\033[2;1Htime: %ds",(int)(time/24));
}

void print_player(){
	int y;
	if(game.jump>0 && game.jump<6) y=game.jump;
	else if(game.jump>=6) y=10-game.jump;
	else y=game.jump;
	printf("\033[;H\033[0;33;40m");
	printf("\033[%d;7H|",19-y);
	printf("\033[%d;7H|",18-y);
	printf("\033[%d;7Ho",17-y);
}

void print_blocks(){
	printf("\033[;H\033[0;32;40m");
	for(int i=0;i<3;i++){
		if(game.blocks[i]>0 && game.blocks[i]<83){
			printf("\033[%d;%dH=",20,game.blocks[i]);
			printf("\033[%d;%dH|",19,game.blocks[i]+1);
			printf("\033[%d;%dH|",19,game.blocks[i]-1);
			printf("\033[%d;%dH=",18,game.blocks[i]);
		}
	}
}

void print_result(){
	printf("\033[2J\033[;H\033[0;36;40m");
	printf("\033[9;38Hblock passed: %d",pass);
	printf("\033[10;38Htime: %ds",(int)(time/24));
	printf("\033[12;20Hpress any key to enter start menu, press esc to exit");
}


void update_game(){
	int y;
	switch(game.state){
	case 0:
		break;
	case 1:
		time++;
		if(game.jump>=10) game.jump=0;
		if(game.jump!=0) game.jump++;
		for(int i=0;i<3;i++){
			game.blocks[i]--;
			if(game.blocks[i]<=0){
				game.blocks[i]=82+rand()%82;
			}
			if(game.blocks[i]==7){
				switch(game.jump){
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
					y=game.jump;
					break;
				default:
					y=10-game.jump;
				}
				if(y<3){
					game.state=2;
					game.speed=3;
					game.jump=-1;
					play_music(1);
				}
				else{
					pass++;
				}
			}
		}
		break;
	case 2:
		game.jump--;
		if(game.jump<-10){
			game.state=3;
			game.speed=0;
			game.jump=0;
			game.blocks[0]=80;
			game.blocks[1]=110;
			game.blocks[2]=130;
		}
	}
}
