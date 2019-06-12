#include "bus.h"

const dword ARM_RD = 1 << 8;
const dword ARM_WR = 1 << 9;
const dword ARM_ALE = 1 << 10;

#define ARM_WAIT() ((GPIO2->PD & (1 << 11)) == 0)

//---------------------------------------------------------------------------------------
bool SystemBus_TestConfiguration()
{
	portENTER_CRITICAL();

	SystemBus_SetAddress(0xc000f0);

	GPIO2->PM = ~(ARM_RD | ARM_WR | ARM_ALE);
	GPIO2->PD = (ARM_RD | ARM_WR | ARM_ALE);

	GPIO0->PM = ~0xffff0000;
	GPIO0->PC2 = 0x00000000;
	GPIO0->PD = 0xffff0000;

	GPIO2->PM = ~ARM_RD;
	GPIO2->PD = 0;

	DelayUs(10);

	word result = 0;
	if (!ARM_WAIT())
		result = GPIO0->PD >> 16;

	GPIO2->PD = ARM_RD;
	GPIO0->PC2 = 0xffff0000;

	portEXIT_CRITICAL();

	return (result == 0xf125);
}
//---------------------------------------------------------------------------------------
void SystemBus_SetAddress(dword address)
{
	portENTER_CRITICAL();

	GPIO0->PM = ~0xffff0000;
	GPIO2->PM = ~0x000007ff;

	GPIO0->PD = (address & 0xffff) << 16;
	GPIO2->PD = (ARM_RD | ARM_WR | ARM_ALE) | ((address >> 16) & 0xff);

	GPIO2->PM = ~ARM_ALE;
	GPIO2->PD = 0;
	GPIO2->PD = ARM_ALE;

	portEXIT_CRITICAL();

	WDT_Kick();
}
//---------------------------------------------------------------------------------------
word SystemBus_Read()
{
	portENTER_CRITICAL();

	GPIO0->PM = ~0xffff0000;
	GPIO0->PC2 = 0x00000000; // input
	// GPIO0->PC1 = 0xffff0000;
	// GPIO0->PC0 = 0xffff0000;
	GPIO0->PD = 0xffff0000;

	GPIO2->PM = ~ARM_RD;
	GPIO2->PD = 0;

	while (ARM_WAIT());
	word result = GPIO0->PD >> 16;

	GPIO2->PD = ARM_RD;

	GPIO0->PC2 = 0xffff0000; // input
	// GPIO0->PC1 = 0x00000000;
	// GPIO0->PC0 = 0xffff0000;

	portEXIT_CRITICAL();

	return result;
}
//---------------------------------------------------------------------------------------
word SystemBus_Read(dword address)
{
	word result;

	portENTER_CRITICAL();
	SystemBus_SetAddress(address);
	result = SystemBus_Read();
	portEXIT_CRITICAL();

	return result;
}
//---------------------------------------------------------------------------------------
void SystemBus_Write(word data)
{
	portENTER_CRITICAL();

	GPIO0->PM = ~0xffff0000;
	GPIO2->PM = ~ARM_WR;

	GPIO0->PD = data << 16;
	GPIO2->PD = 0;

	while (ARM_WAIT());
	GPIO2->PD = ARM_WR;

	portEXIT_CRITICAL();
}
//---------------------------------------------------------------------------------------
void SystemBus_Write(dword address, word data)
{
	portENTER_CRITICAL();

	SystemBus_SetAddress(address);
	SystemBus_Write(data);

	portEXIT_CRITICAL();
}
//---------------------------------------------------------------------------------------
