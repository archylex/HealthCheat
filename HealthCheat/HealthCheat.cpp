// HealthCheat.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <stdlib.h>

using namespace std;

DWORD getProcessID(wstring procName) {
	DWORD procId = 0;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof entry;

	if (!Process32FirstW(snap, &entry))
		return 0;

	do {
		if (wstring(entry.szExeFile) == procName) {
			procId = entry.th32ProcessID;
			break;
		}
	} while (Process32NextW(snap, &entry));

	CloseHandle(snap);

	return procId;
}

DWORD getModuleBaseAddress(DWORD procId, wstring modName) {
	DWORD modBaseAddr = 0;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procId);

	if (snap != INVALID_HANDLE_VALUE) {
		MODULEENTRY32W modEntry;
		modEntry.dwSize = sizeof(modEntry);

		if (Module32FirstW(snap, &modEntry)) {
			do {
				if (wstring(modEntry.szModule) == modName) {
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32NextW(snap, &modEntry));
		}
	}

	CloseHandle(snap);

	return modBaseAddr;
}

DWORD getAddress(HANDLE hProc, DWORD pointer, vector<unsigned int> offsets) {
	DWORD address = pointer;

	for (unsigned int i = 0; i < offsets.size(); ++i) {
		ReadProcessMemory(hProc, (BYTE*)address, &address, sizeof address, 0);
		address += offsets[i];
	}

	return address;
}

DWORD getPointerAddress(DWORD modAddr, DWORD offset) {
	return modAddr + offset;
}

int main() {
	DWORD dynamicOffset = 0xA4931C;
	vector<unsigned int> healthOffsets = { 0x230 };
	vector<unsigned int> armorOffsets = { 0xF78 };
	int healthValue = 0;
	int newHealthValue = 999;
	int armorValue = 0;
	int newArmorValue = 255;

	DWORD procId = getProcessID(L"csgo.exe");

	cout << "*** Starting..." << endl;

	if (procId > 0) {
		cout << "Process ID: " << procId << endl;

		DWORD modBase = getModuleBaseAddress(procId, L"server.dll");

		if (modBase > 0) {
			cout << "Module base address: 0x" << hex << modBase << endl;

			DWORD dynamicAddr = getPointerAddress(modBase, dynamicOffset);
			cout << "Dynamic address: 0x" << hex << dynamicAddr << endl;

			HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);

			DWORD healthAddr = getAddress(hProc, dynamicAddr, healthOffsets);
			DWORD armorAddr = getAddress(hProc, dynamicAddr, armorOffsets);

			cout << "*** Memory addresses:" << endl;
			cout << "Health: 0x" << hex << healthAddr << endl;
			cout << "Armor: 0x" << hex << healthAddr << endl;

			ReadProcessMemory(hProc, (BYTE*)(healthAddr), &healthValue, sizeof healthValue, nullptr);
			ReadProcessMemory(hProc, (BYTE*)(armorAddr), &armorValue, sizeof armorValue, nullptr);

			cout << "*** Values:" << endl;
			cout << "Health: " << dec << healthValue << endl;
			cout << "Armor: " << dec << armorValue << endl;

			cout << "*** Writing new value..." << endl;
			WriteProcessMemory(hProc, (BYTE*)(healthAddr), &newHealthValue, sizeof newHealthValue, 0);
			WriteProcessMemory(hProc, (BYTE*)(armorAddr), &newArmorValue, sizeof newArmorValue, 0);

			ReadProcessMemory(hProc, (BYTE*)(healthAddr), &healthValue, sizeof healthValue, nullptr);
			ReadProcessMemory(hProc, (BYTE*)(armorAddr), &armorValue, sizeof armorValue, nullptr);

			cout << "*** New values:" << endl;
			cout << "Health: " << dec << healthValue << endl;
			cout << "Armor: " << dec << armorValue << endl;
		}
		else {
			cout << "*** The module base address wasn't found." << endl;
		}
	}
	else {
		cout << "*** The process wasn't found." << endl;
	}

	getchar();

	return 0;
}