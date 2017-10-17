*** Settings ***
Library		OperatingSystem
Library		XTFLibrary

*** Keywords ***
Run DomU
	[Arguments]			${cfg}
	File Should Exist		${cfg}
	${domid}=			XTFLibrary.Create	${cfg}
	XTFLibrary.Resume		${domid}
	XTFLibrary.WaitForPattern	${domid}		SUCCESS		10
	XTFLibrary.Cleanup		${domid}		10


