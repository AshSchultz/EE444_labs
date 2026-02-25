//****************************************************************************//
// Decrement Core Voltage
//
// D. Raskovic
//    for use in EE444/645 only
//
// adapted from hal_PMM.c by TI
// changed to make it safer (checks for boundaries) 
//****************************************************************************//

#include <msp430.h>

#define _HAL_PMM_DISABLE_SVML_
#define _HAL_PMM_DISABLE_SVSL_
#define _HAL_PMM_DISABLE_FULL_PERFORMANCE_

//****************************************************************************//
#ifdef _HAL_PMM_DISABLE_SVML_
#define _HAL_PMM_SVMLE  (SVMLE)
#else
#define _HAL_PMM_SVMLE  (0)
#endif
#ifdef _HAL_PMM_DISABLE_SVSL_
#define _HAL_PMM_SVSLE  (SVSLE)
#else
#define _HAL_PMM_SVSLE  (0)
#endif
#ifdef _HAL_PMM_DISABLE_FULL_PERFORMANCE_
#define _HAL_PMM_SVSFP  (SVSLFP)
#define _HAL_PMM_SVMFP  (SVMLFP)
#else
#define _HAL_PMM_SVSFP  (0)
#define _HAL_PMM_SVMFP  (0)
#endif


int DecrementVcore (void)
{
  unsigned int PMMRIE_backup;
  unsigned short level, old_level; 

  // Open PMM registers for write access
  PMMCTL0_H = 0xA5;

  // Disable dedicated Interrupts to prevent that needed flags will be cleared
  PMMRIE_backup = PMMRIE;
  PMMRIE &= ~(SVMHVLRPE | SVSHPE | SVMLVLRPE | SVSLPE | SVMHVLRIE \
  				| SVMHIE | SVSMHDLYIE | SVMLVLRIE | SVMLIE | SVSMLDLYIE );

 // read the previous Vcore level 
  old_level = PMMCTL0 & (PMMCOREV_3); 
  if (old_level > 0) 
    level = --old_level; 

  // Set SVM high side and SVM low side to new level
  PMMIFG &= ~(SVMHIFG | SVSMHDLYIFG | SVMLIFG | SVSMLDLYIFG);
  SVSMHCTL = SVMHE | SVMHFP | (SVSMHRRL0 * level);
  SVSMLCTL = SVMLE | SVMLFP | (SVSMLRRL0 * level);
  // Wait until SVM high side and SVM low side is settled
  while ((PMMIFG & SVSMHDLYIFG) == 0 || (PMMIFG & SVSMLDLYIFG) == 0);

  // Set VCore to new level
  PMMCTL0_L = PMMCOREV0 * level;

  // Set also SVS highside and SVS low side to new level
  PMMIFG &= ~(SVSHIFG | SVSMHDLYIFG | SVSLIFG | SVSMLDLYIFG);
  SVSMHCTL |= SVSHE | SVSHFP | (SVSHRVL0 * level);
  SVSMLCTL |= SVSLE | SVSLFP | (SVSLRVL0 * level);
  // Wait until SVS high side and SVS low side is settled
  while ((PMMIFG & SVSMHDLYIFG) == 0 || (PMMIFG & SVSMLDLYIFG) == 0);
  // Disable full-performance mode to save energy
  SVSMHCTL &= ~_HAL_PMM_SVSFP;
  // Disable SVS/SVM Low
  // Disable full-performance mode to save energy
  SVSMLCTL &= ~(_HAL_PMM_SVSLE + _HAL_PMM_SVMLE + _HAL_PMM_SVSFP + _HAL_PMM_SVMFP);
  SVSMHCTL &= ~(_HAL_PMM_SVSFP + _HAL_PMM_SVMFP);
	
  // Clear all Flags
  PMMIFG &= ~(SVSLIFG | SVSHIFG | PMMPORIFG | PMMRSTIFG | PMMBORIFG	| SVMHVLRIFG \
  				| SVMHIFG | SVSMHDLYIFG | SVMLVLRIFG | SVMLIFG | SVSMLDLYIFG);
  // backup PMM-Interrupt-Register
  PMMRIE = PMMRIE_backup;
  // Lock PMM registers for write access
  PMMCTL0_H = 0x00;

  if ((PMMIFG & SVMHIFG) == SVMHIFG)
    return 0;	        // Highside is still too low for the adjusted VCore Level
  else 
    return 1;		    // Return: OK
}
