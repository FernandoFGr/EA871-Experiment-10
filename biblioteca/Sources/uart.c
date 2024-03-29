/*!
 * \file uart.c
 *
 * \date Feb 16, 2017
 * \author Wu Shin-Ting e Fernando Granado
 */

#include "uart.h"

/*!
 * \fn initUART (void)
 * \brief Inicializa os pinos conectados a UART0 e ao OpenSDA.
 * \return c&oacute;digo de erro (1=taxa inv&aacute;lida; 2=baud inv&aacute;lido; 3=combina&ccedil;&atilde;o inv&aacute;lida)
 */
uint8_t initUART(uint8_t taxa, unsigned int baud_rate) {
	/*!
	 * Configura&ccedil;&atilde;o de SIM
         */

	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
	SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
	SIM_SOPT2 |= SIM_SOPT2_UART0SRC(0x1);	///< configura a fonte de rel�gio (20.971520MHz)
	SIM_SOPT2 &= ~SIM_SOPT2_PLLFLLSEL_MASK;	 
	                                         ///< no reset, o valor � zero (FLL)
	                                         ///< mas n�o deixa de ser uma boa
	                                         ///< pr�tica de inicializar explicitamente
	
	/*!
	 * Configura&ccedil;&atilde;o dos pinos que servem UART0 
     */
	PORTA_PCR1 |= PORT_PCR_MUX(0b010);         ///< Configure pino com fun&ccedil;&atilde;o UART0_RX
	PORTA_PCR2 |= PORT_PCR_MUX(0b010);         ///< Configure pino com fun&ccedil;&atilde;o UART0_TX
		
	/*!
	 * Configura&ccedil;&atilde;o do m&oacute;dulo UART0 
         */
	UART0_C1 &= ~(UART0_C1_PE_MASK |			///< Configure "no parity"
			UART0_C1_ILT_MASK |
			UART0_C1_WAKE_MASK |
			UART0_C1_M_MASK |                   ///< Configure "dados de 8 bits"
			UART0_C1_DOZEEN_MASK |
			UART0_C1_LOOPS_MASK );
	/*!
	 * Desabilita canal receptor e transmissor para configurar baud rate
	 */
	UART0_C2 &= ~(UART_C2_RE_MASK |           
				UART_C2_TE_MASK);
	UART0_BDH &= ~(UART_BDH_SBNS_MASK | 
				UART_BDH_RXEDGIE_MASK | 
				UART_BDH_LBKDIE_MASK);
	
	/*!
	 * Valida o valor da taxa 
	 */
	if (taxa < 4 || taxa > 32) return 1;
	
	/*!
	 * Habilite BOTHEDGE se taxa de superamostragem � x4 a x7
	 */
	if (taxa < 8)
		UART0_C5 |= UART0_C5_BOTHEDGE_MASK;       ///< amostra as duas bordas nos dados do receptor (entre taxa de amostragem 4x e 7x)

	/*!
	 * Taxa de superamostragem. O padr&atilde;o &eacute; 0x0F e os valores devem ser maiores que 3
	 */
	UART0_C4 |= (UART0_C4_OSR(0b11111));
	UART0_C4 &= (UART0_C4_OSR(taxa-1));           ///< Resetar o campo 

	/*! 
	 * Valida o valor de baud
	 */
	if (baud_rate != 300 && baud_rate != 1200 && baud_rate != 2400 && baud_rate != 4800 
			&& baud_rate != 9600 && baud_rate != 19200 && baud_rate != 38400 
			&& baud_rate != 57600 && baud_rate != 115200) return 2;
	
	/*!
	 * Computar SBR v&aacute;lido
	 */
	uint32_t SBR = (uint32_t)(20971520./(taxa * baud_rate)); /*! SBR eh quanto se deve dividir a frequencia do clock para obter a frequencia de leitura de bits (considerando as superamostragens) */
	/*! taxa*baud_rate eh a frequencia de leitura de bits (considerando as superamostragens) */
	/*! taxa eh o numero de superamostragens */
	if (SBR > 0x1FFF) return 3;
	
	/*!
	 * Setar SBR (Se&ccedil;&atilde;o 8.3.2 em 
	 * ftp://ftp.dca.fee.unicamp.br/pub/docs/ea871/ARM/KLQRUG.pdf)
	 */
	UART0_BDH &= ~UART_BDH_SBR(0b11111);                             
	UART0_BDL &= ~UART_BDL_SBR(0b11111111);            
	UART0_BDH |= UART_BDH_SBR((SBR & 0x1F00)>>8);                             
	UART0_BDL |= UART_BDL_SBR(SBR & 0x00FF);            
		
	UART0_S1 |= (UART0_S1_PF_MASK |              ///< Registradores de estado: w1c
				UART0_S1_FE_MASK |
				UART0_S1_NF_MASK |
				UART0_S1_OR_MASK |
				UART0_S1_IDLE_MASK);
	
	UART0_C2 |= (UART_C2_RE_MASK |         		///< habilita o canal receptor 
				 UART_C2_TE_MASK);	  			///< habilita o canal transmissor  
	
	return 0;
}

/*!
 * \brief Habilita a interrup&ccedil;&atilde;o do canal receptor
 */
void enableRIEInterrup (void) {
	UART0_C2 |= UART_C2_RIE_MASK;
	
}

/*!
 * \brief Habilita a interrup&ccedil;&atilde;o do canal transmissor
 */
void enableTEInterrup (void) {
	UART0_C2 |= UART_C2_TIE_MASK;
}

/*!
 * \brief Desabilita a interrup&ccedil;&atilde;o do canal receptor
 */
void disableRIEInterrup (void) {
	UART0_C2 &= ~UART_C2_RIE_MASK;
}

/*!
 * \brief Desabilita a interrup&ccedil;&atilde;o do canal transmissor
 */
void disableTEInterrup (void) {
	UART0_C2 &= ~UART_C2_TIE_MASK;
	
}

/*!
 * \brief Habilita o canal receptor
 */
void enableRE (void) {
	UART0_C2 |= UART_C2_RE_MASK;
	
}

/*!
 * \brief Habilita o canal transmissor
 */
void enableTE (void) {
	UART0_C2 |= UART_C2_TE_MASK;
}

/*!
 * \brief Desabilita o canal receptor
 */
void disableRE (void) {
	UART0_C2 &= ~UART_C2_RE_MASK;
}

/*!

 * \brief Desabilita o canal transmissor

 */
void disableTE (void) {
	UART0_C2 &= ~UART_C2_TE_MASK;
	
}
