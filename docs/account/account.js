'use strict';
const query=new URLSearchParams(location.search);
const language=['pl','en'].includes(query.get('lang'))?query.get('lang'):(navigator.language.toLowerCase().startsWith('pl')?'pl':'en');
const messages={
 pl:{title:'Konto VoxLane',email:'E-mail',password:'Hasło',login:'Zaloguj',signup:'Utwórz konto',forgot:'Nie pamiętasz hasła?',newPassword:'Nowe hasło',save:'Zapisz',back:'Wróć',confirm:'Potwierdź e-mail',reset:'Zmień hasło',inbox:'Sprawdź pocztę.',verified:'E-mail potwierdzony.',changed:'Hasło zmienione.',logged:'Zalogowano jako',shortPassword:'Minimum 10 znaków.',incorrect:'Nieprawidłowy e-mail lub hasło.',expired:'Link wygasł. Poproś o nowy.',network:'Sprawdź połączenie z internetem.',error:'Nie udało się. Spróbuj ponownie.',emailRequired:'Podaj e-mail.',wait:'Spróbuj ponownie za chwilę.',confirmFirst:'Potwierdź e-mail.',success:'Zakup przyjęty. W aplikacji kliknij „Sprawdź zakup”.',cancel:'Zakup anulowany.',test:'Płatności testowe',price:'Pro · 25 zł jednorazowo'},
 en:{title:'VoxLane account',email:'Email',password:'Password',login:'Log in',signup:'Create account',forgot:'Forgot password?',newPassword:'New password',save:'Save',back:'Back',confirm:'Confirm email',reset:'Reset password',inbox:'Check your inbox.',verified:'Email confirmed.',changed:'Password changed.',logged:'Logged in as',shortPassword:'At least 10 characters.',incorrect:'Incorrect email or password.',expired:'Link expired. Request a new one.',network:'Check your internet connection.',error:'Could not complete this action. Try again.',emailRequired:'Enter your email.',wait:'Try again shortly.',confirmFirst:'Confirm your email.',success:'Purchase received. Click “Check purchase” in the app.',cancel:'Purchase cancelled.',test:'Test payments',price:'Pro · PLN 25 once'}
};
const t=key=>messages[language][key],status=document.querySelector('#status');
let cfg,access='';
document.documentElement.lang=language;document.title=t('title');
for(const e of document.querySelectorAll('[data-i18n]'))e.textContent=t(e.dataset.i18n);
function show(message){status.textContent=message;}
function errorMessage(v){
 const code=v.error_code||v.code||'',msg=v.msg||v.error_description||v.error||'';
 if(code==='email_not_confirmed'||/not confirmed/i.test(msg))return t('confirmFirst');
 if(/credentials/i.test(code)||/invalid login/i.test(msg))return t('incorrect');
 if(code==='weak_password')return t('shortPassword');
 if(/rate_limit/.test(code))return t('wait');
 if(code==='otp_expired'||/expired|invalid.*token/i.test(msg))return t('expired');
 return t('error');
}
async function request(path,body,method='POST'){
 if(!cfg)throw Error(t('error'));
 let response;
 try{response=await fetch(cfg.supabaseUrl.replace(/\/$/,'')+path,{method,headers:{'Content-Type':'application/json',apikey:cfg.supabasePublicKey,...(access?{Authorization:'Bearer '+access}:{})},body:JSON.stringify(body)});}catch{throw Error(t('network'));}
 const value=await response.json();if(!response.ok)throw Error(errorMessage(value));return value;
}
function values(){return {email:document.querySelector('#email').value.trim(),password:document.querySelector('#password').value};}
async function action(fn){
 for(const b of document.querySelectorAll('button'))b.disabled=true;
 try{await fn();}catch(e){show(e.message);}finally{for(const b of document.querySelectorAll('button'))b.disabled=false;}
}
fetch('public-config.json',{cache:'no-store'}).then(r=>{if(!r.ok)throw Error();return r.json();}).then(c=>{if(!c.supabaseUrl.startsWith('https://'))throw Error();cfg=c;}).catch(()=>show(t('error')));
function redirectPath(path){const url=new URL(cfg.accountPageUrl||location.origin+location.pathname);url.searchParams.set('lang',language);return path+'?redirect_to='+encodeURIComponent(url.href);}
document.querySelector('#account').onsubmit=e=>{e.preventDefault();action(async()=>{
 const session=await request('/auth/v1/token?grant_type=password',values());access=session.access_token;
 if(!session.user?.email)throw Error(t('error'));
 document.querySelector('#password').value='';document.querySelector('#email').value=session.user.email;
 show(t('logged')+' '+session.user.email+'.');
 await request('/auth/v1/user',{data:{language}},'PUT');
});};
document.querySelector('#signup').onclick=()=>action(async()=>{
 const v=values();if(!v.email)throw Error(t('emailRequired'));if(v.password.length<10)throw Error(t('shortPassword'));
 await request(redirectPath('/auth/v1/signup'),{...v,data:{language}});document.querySelector('#password').value='';show(t('inbox'));
});
document.querySelector('#forgot').onclick=()=>action(async()=>{
 const email=values().email;if(!email)throw Error(t('emailRequired'));
 await request(redirectPath('/auth/v1/recover'),{email});show(t('inbox'));
});
const hash=new URLSearchParams(location.hash.slice(1));
const link={token:hash.get('token_hash'),type:hash.get('type')};
if(link.token&&['signup','recovery'].includes(link.type)){
 history.replaceState(null,'',location.pathname+location.search);
 document.querySelector('#account').hidden=true;document.querySelector('#email-action').hidden=false;
 document.querySelector('#verify-email').textContent=t(link.type==='recovery'?'reset':'confirm');
}else if(hash.get('type')==='recovery'&&hash.get('access_token')){
 access=hash.get('access_token');history.replaceState(null,'',location.pathname+location.search);
 document.querySelector('#recovery').hidden=false;document.querySelector('#account').hidden=true;
}else{
 if(hash.has('access_token')||hash.has('refresh_token')||hash.has('error'))history.replaceState(null,'',location.pathname+location.search);
 if(hash.get('type')==='signup'&&hash.has('access_token'))show(t('verified'));
 if(hash.has('error'))show(t('expired'));
 const payment=query.get('payment');if(payment==='success')show(t('success'));if(payment==='cancelled')show(t('cancel'));
}
document.querySelector('#email-action').onsubmit=e=>{e.preventDefault();action(async()=>{
 const session=await request('/auth/v1/verify',{token_hash:link.token,type:link.type});
 if(!session.access_token||!session.user?.id)throw Error(t('expired'));
 link.token='';document.querySelector('#email-action').hidden=true;
 if(link.type==='recovery'){
  access=session.access_token;document.querySelector('#recovery').hidden=false;
 }else{
  access='';document.querySelector('#account').hidden=false;document.querySelector('#email').value=session.user.email;show(t('verified'));
 }
});};
document.querySelector('#email-action-cancel').onclick=()=>{
 link.token='';document.querySelector('#email-action').hidden=true;document.querySelector('#account').hidden=false;show('');
};
document.querySelector('#recovery').onsubmit=e=>{e.preventDefault();action(async()=>{
 const password=document.querySelector('#new-password').value;if(password.length<10)throw Error(t('shortPassword'));
 await request('/auth/v1/user',{password,data:{language}},'PUT');document.querySelector('#new-password').value='';access='';
 document.querySelector('#recovery').hidden=true;document.querySelector('#account').hidden=false;show(t('changed'));
});};
