R"(
var contextPanel = $.GetContextPanel();
for (var dnBackground of contextPanel.FindChildrenWithClassTraverse('DeathNoticeBGBorder')) {

	dnBackground.style.border = '0px solid #00000000';
	dnBackground.style.boxShadow = 'inset #00000000 0px 0px 0px 0px;';
	dnBackground.style.borderRadius = '5px';
	dnBackground.style.height = '110%';
	dnBackground.style.width = '105%';
	dnBackground.style.paddingTop = "0px";
	dnBackground.style.height = '36px';
}

for (var dnBackgroundg of contextPanel.FindChildrenWithClassTraverse('DeathNoticeBG')) {
	dnBackgroundg.style.backgroundImage = 'url("https://images2.imgbox.com/49/45/mrwub2K0_o.png")';
	dnBackgroundg.style.backgroundSize = '100% 100%';
	dnBackgroundg.style.backgroundColor = '#00000000';
	dnBackgroundg.style.opacity = '0.7';
}

for (var dnBackgroundgradient of contextPanel.FindChildrenWithClassTraverse('DeathNoticeBGGradient')) {
	dnBackgroundgradient.style.height = '100%';
	dnBackgroundgradient.style.width = '100%';
	dnBackgroundgradient.opacity = '0.76';
	dnBackgroundgradient.verticalAlign = 'center';
	dnBackgroundgradient.style.boxShadow = 'inset #000000 0px 0px 2px';
}


for (var dnIcon of contextPanel.FindChildrenWithClassTraverse('DeathNoticeIcon')) {
	if (dnIcon.id === "Weapon") {
		dnIcon.style.height = '24px';
		dnIcon.style.verticalAlign = 'top';
		dnIcon.style.uiScale = "70%";
		dnIcon.style.transform = 'translateY(2px)';	
		dnIcon.style.margin = '0px 5px 0px 5px';

	} else {
		dnIcon.style.height = '22px';
		dnIcon.style.verticalAlign = 'top';
		dnIcon.style.uiScale = "80%";
		dnIcon.style.margin = '0px 5px 0px 5px';

	}
}

for (var dnT of contextPanel.FindChildrenWithClassTraverse('DeathNoticeTColor')) {
	dnT.style.color = '#f5ca68';
	dnT.style.textShadow = '0px 0px 0px #00000000';
	dnT.style.fontFamily = 'Stratum2, "Arial Unicode MS"';
	dnT.style.fontSize = '21px';
	dnT.style.fontWeight = 'bold';
	dnT.style.letterSpacing = '0.20px';
	dnT.style.verticalAlign = 'bottom';
	dnT.style.textOverflow = 'ellipsis';
	dnT.style.transform = 'scaleX(0.99) scaleY(1.03) translateY(1.9px)';
	dnT.style.S2MixBlendMode = 'SRGBadditive';
}
for (var dnCt of contextPanel.FindChildrenWithClassTraverse('DeathNoticeCTColor')) {
	dnCt.style.color = '#98abd5';
	dnCt.style.textShadow = '0px 0px 0px #00000000';
	dnCt.style.fontFamily = 'Stratum2, "Arial Unicode MS"';
	dnCt.style.fontSize = '21px';
	dnCt.style.fontWeight = 'bold';
	dnCt.style.letterSpacing = '0.20px';
	dnCt.style.verticalAlign = 'bottom';
	dnCt.style.textOverflow = 'ellipsis';
	dnCt.style.transform = 'scaleX(0.99) scaleY(1.03) translateY(1.9px)';
	dnCt.style.S2MixBlendMode = 'SRGBadditive';
}


for (var deathnotice of contextPanel.FindChildrenWithClassTraverse('DeathNotice')) {
	deathnotice.FindChildTraverse('NoScopeIcon').style.visibility = 'collapse';
	deathnotice.FindChildTraverse('ThroughSmokeIcon').style.visibility = 'collapse';
	deathnotice.FindChildTraverse('AttackerBlindIcon').style.visibility = 'collapse';
	deathnotice.FindChildTraverse('Domination').style.visibility = 'collapse';

	deathnotice.style.margin = '0px -8px 1px 18px';

	
	if (deathnotice.BHasClass('DeathNotice_Killer')) {
		for (var killer of contextPanel.FindChildTraverse('HudDeathNotice').FindChildrenWithClassTraverse('DeathNotice_Killer')) {
			for (var content of killer.FindChildrenWithClassTraverse('DeathNoticeBG')) {
				content.style.border = '1.5px solid #b3070d;';
				content.style.borderRadius = '3px';
				content.style.backgroundColor = 'black';
				content.style.opacity = '0.8';
			}
		}
	}
	if (deathnotice.BHasClass('DeathNotice_Victim')) {
		for (var victim of contextPanel.FindChildTraverse('HudDeathNotice').FindChildrenWithClassTraverse('DeathNotice_Victim')) {
			for (var content of victim.FindChildrenWithClassTraverse('DeathNoticeBG')) {

				content.style.backgroundColor = '#a81313';
				content.style.borderRadius = '3px';
				content.style.boxShadow = 'inset #e10000e6 0px 0px 1px;';
				content.style.opacity = '1';
			}
		}
	}
}


)"