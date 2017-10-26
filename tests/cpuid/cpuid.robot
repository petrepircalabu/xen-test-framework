*** Settings ***
Resource	../common.robot

*** Test Cases ***
test-hvm32-cpuid
	[Tags]		hvm32
	Run DomU	${CURDIR}/test-hvm32-cpuid.cfg

test-hvm32pae-cpuid
	[Tags]		hvm32pae
	Run DomU	${CURDIR}/test-hvm32pae-cpuid.cfg

test-hvm32pse-cpuid
	[Tags]		hvm32pse
	Run DomU	${CURDIR}/test-hvm32pse-cpuid.cfg

test-hvm64-cpuid
	[Tags]		hvm64
	Run DomU	${CURDIR}/test-hvm64-cpuid.cfg

test-pc32pae-cpuid
	[Tags]		pv32pae
	Run DomU	${CURDIR}/test-pv32pae-cpuid.cfg

test-pv64-cpuid
	[Tags]		pv64
	Run DomU	${CURDIR}/test-pv64-cpuid.cfg
