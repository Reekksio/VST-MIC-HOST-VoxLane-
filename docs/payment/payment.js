'use strict';
const query=new URLSearchParams(location.search);
const lang=['pl','en'].includes(query.get('lang'))?query.get('lang'):(navigator.language.toLowerCase().startsWith('pl')?'pl':'en');
const text={pl:{title:'Status płatności',success:'Wróć do VoxLane i kliknij „Sprawdź zakup”. Pro zostanie aktywowane po potwierdzeniu płatności.',cancelled:'Zakup anulowany. Możesz wrócić do VoxLane i spróbować ponownie.',price:'Pro · 25 zł jednorazowo · Bez subskrypcji',home:'Strona VoxLane'},en:{title:'Payment status',success:'Return to VoxLane and click “Check purchase”. Pro activates after payment verification.',cancelled:'Purchase cancelled. You can return to VoxLane and try again.',price:'Pro · PLN 25 once · No subscription',home:'VoxLane website'}}[lang];
document.documentElement.lang=lang;document.title='VoxLane · '+text.title;
for(const id of ['title','price','home'])document.getElementById(id).textContent=text[id];
document.getElementById('status').textContent=query.get('payment')==='cancelled'?text.cancelled:text.success;
